/**
 * @file mmc_module.c
 * @brief TODO.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <sel4/sel4.h>
#include <sel4debug/debug.h>
#include <allocman/vka.h>
#include <platsupport/delay.h>
#include <sel4utils/sel4_zf_logif.h>
#include <sdhc/sdio.h>
#include <sdhc/mmc.h>

#include <fat/fat_filelib.h>

#include "config.h"
#include "init_env.h"
#include "thread.h"
#include "mmc_entry.h"
#include "mmc.h"
#include "mmc_module.h"

#define FATIO_MEDIA_RW_FAILURE (0)
#define FATIO_MEDIA_RW_SUCCESS (1)

static sdio_host_dev_t g_sdio_dev;
static mmc_card_t g_mmc_card;

static seL4_Word g_msg_rx_buffer[seL4_MsgMaxLength];

static thread_s g_thread;
static uint64_t g_thread_stack[MMCMOD_STACK_SIZE];

static const char FILE_NAME[] = "/hobd.log";

/* Align 'n' up to the value 'align', which must be a power of two. */
static seL4_Word align_up(
        const seL4_Word n,
        const seL4_Word align)
{
    return (n + align - 1) & (~(align - 1));
}

/* FAT IO media API */
#ifndef SIMULATION_BUILD
static int fatio_media_read(
        unsigned long sector,
        unsigned char *buffer,
        unsigned long sector_count)
{
    int ret = FATIO_MEDIA_RW_SUCCESS;

    const long bytes_read = mmc_block_read(
            g_mmc_card,
            sector,
            (int) sector_count,
            (void*) buffer,
            0,
            NULL,
            NULL);

    if(bytes_read != ((long) sector_count * FAT_SECTOR_SIZE))
    {
        ret = FATIO_MEDIA_RW_FAILURE;
    }

    return ret;
}

/* FAT IO media API */
static int fatio_media_write(
        unsigned long sector,
        unsigned char *buffer,
        unsigned long sector_count)
{
    int ret = FATIO_MEDIA_RW_SUCCESS;

    const long bytes_written = mmc_block_write(
            g_mmc_card,
            sector,
            (int) sector_count,
            (void*) buffer,
            0,
            NULL,
            NULL);

    if(bytes_written != ((long) sector_count * FAT_SECTOR_SIZE))
    {
        ret = FATIO_MEDIA_RW_FAILURE;
    }

    return ret;
}
#endif

static void mmc_thread_fn(const seL4_CPtr ep_cap)
{
    seL4_Word badge;

    /* initialize FAT IO library */
    fl_init();

#ifndef SIMULATION_BUILD
    int err;
    const unsigned long long card_cap = mmc_card_capacity(g_mmc_card);
    ZF_LOGD("MMC card capacity = %llu bytes", card_cap);

    /* attach media access functions */
    err = fl_attach_media(&fatio_media_read, &fatio_media_write);
    ZF_LOGF_IF(err != FAT_INIT_OK, "Failed to attach FAT IO media access functions");
#endif

    ZF_LOGD(MMCMOD_THREAD_NAME " thread is running - ep_cap = 0x%X", ep_cap);

    while(1)
    {
        /* TODO - handle incoming IPC/messages to be logged to the MMC card */
        /* TODO - keep file handle open, flush on a timeout? */

        const seL4_MessageInfo_t info = seL4_Recv(
                ep_cap,
                &badge);

        ZF_LOGF_IF(badge != (MMCMOD_EP_BADGE + 1), "Invalid IPC badge");

        const seL4_Word total_size_words = seL4_MessageInfo_get_length(info);

        /* reuse badge as index , this is probably unnecessary */
        for(badge = 0; badge < total_size_words; badge += 1)
        {
            g_msg_rx_buffer[badge] = seL4_GetMR(badge);
        }

        const mmc_entry_s * const entry =
                (const mmc_entry_s*) &g_msg_rx_buffer[0];

        ZF_LOGF_IF(entry->type == MMC_ENTRY_TYPE_INVALID, "Invalid entry type");
        ZF_LOGF_IF(entry->size > MMC_ENTRY_DATA_SIZE_MAX, "Entry size is too large");

#ifndef SIMULATION_BUILD
        FL_FILE * const file = fl_fopen(FILE_NAME, "wba");

        if(file != NULL)
        {
            const uint16_t entry_size_bytes = entry->size + MMC_ENTRY_HEADER_SIZE;

            err = fl_fwrite((void*) entry, 1, entry_size_bytes, file);
            ZF_LOGF_IF(err != (int) entry_size_bytes, "Failed to write to MMC card");

            fl_fclose(file);
        }
        else
        {
            ZF_LOGW("failed to open MMC card file");
        }
#endif
    }

    /* cleanup and release FAT IO library */
    fl_shutdown();

    /* should not get here, intentional halt */
    seL4_DebugHalt();
}

static void init_sdio(
        init_env_s * const env)
{
    const int err = sdio_init(sdio_default_id(), &env->io_ops, &g_sdio_dev);
    ZF_LOGF_IF(err != 0, "Failed to initialize SDIO device");
}

static void init_mmc(
        init_env_s * const env)
{
    (void) memset(&g_mmc_card, 0, sizeof(g_mmc_card));

#ifndef SIMULATION_BUILD
    const int err = mmc_init(&g_sdio_dev, &env->io_ops, &g_mmc_card);
    ZF_LOGF_IF(err != 0, "Failed to initialize MMC card");
#endif
}

void mmc_module_init(
        init_env_s * const env)
{
    init_sdio(env);
    init_mmc(env);

    ZF_LOGF_IF(
            FAT_SECTOR_SIZE != mmc_block_size(g_mmc_card),
            "MMC block size does not match FAT sector size");

    /* create a worker thread */
    thread_create(
            MMCMOD_THREAD_NAME,
            MMCMOD_EP_BADGE,
            (uint32_t) sizeof(g_thread_stack),
            &g_thread_stack[0],
            &mmc_thread_fn,
            env,
            &g_thread);

    /* set thread priority and affinity */
    thread_set_priority(seL4_MaxPrio, &g_thread);
    thread_set_affinity(MMCMOD_THREAD_AFFINITY, &g_thread);

    ZF_LOGD("%s is initialized", MMCMOD_THREAD_NAME);

    /* start the new thread */
    thread_start(&g_thread);
}

/* TODO */
void mmc_log_entry_data(
        const uint16_t type,
        const uint16_t data_size,
        const uint8_t * const data)
{
    const seL4_Word size_bytes = (seL4_Word) (data_size + MMC_ENTRY_HEADER_SIZE);
    const seL4_Word total_size_bytes = align_up(size_bytes, 4);
    const seL4_Word total_size_words = (seL4_Word) (total_size_bytes / sizeof(seL4_Word));

    ZF_LOGF_IF((total_size_bytes % 4) != 0, "Invalid MMC entry data alignment");

    ZF_LOGF_IF(
            total_size_bytes > MMC_ENTRY_DATA_SIZE_MAX,
            "Failed to log MMC entry data - data size too large");

    const seL4_MessageInfo_t info =
            seL4_MessageInfo_new((seL4_Uint32) type, 0, 0, total_size_words);

    /* header in R0 */
    seL4_SetMR(0, (seL4_Word) (type | (data_size << 16)));

    uint32_t idx;
    for(idx = 0; idx < (total_size_words - 1); idx += 1)
    {
        seL4_Word data_word = 0;

        uint32_t shift;
        for(shift = 0; shift < 4; shift += 1)
        {
            const uint32_t data_index = (idx * 4) + shift;

            if(data_index < (uint32_t) data_size)
            {
                data_word |= (data[data_index] << shift);
            }
        }

        seL4_SetMR(1 + idx, data_word);
    }

    seL4_Send(g_thread.ipc_ep_cap, info);
}
