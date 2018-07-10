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
#include <sync/recursive_mutex.h>
#include <sel4utils/sel4_zf_logif.h>
#include <sdhc/sdio.h>
#include <sdhc/mmc.h>

#include <fat/fat_filelib.h>

#include "config.h"
#include "init_env.h"
#include "thread.h"
#include "time_server.h"
#include "mmc_entry.h"
#include "mmc.h"
#include "mmc_module.h"

#ifdef MMCMOD_DEBUG
#define MODLOGD(...) ZF_LOGD(__VA_ARGS__)
#else
#define MODLOGD(...)
#endif

#define MMC_HARD_FLUSH_TIMEOUT_NS S_TO_NS(2ULL)

#define FATIO_MEDIA_RW_FAILURE (0)
#define FATIO_MEDIA_RW_SUCCESS (1)

static sdio_host_dev_t g_sdio_dev;
static mmc_card_t g_mmc_card;

static FL_FILE *g_file_handle;
static sync_recursive_mutex_t g_fs_mutex;
static uint32_t g_hard_flush_timeout_id;

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

#ifndef SIMULATION_BUILD
/* FAT IO media API */
static void fatio_lock(void)
{
    const int err = sync_recursive_mutex_lock(&g_fs_mutex);
    ZF_LOGF_IF(err != 0, "Failed to lock filesystem mutex");
}

static void fatio_unlock(void)
{
    const int err = sync_recursive_mutex_unlock(&g_fs_mutex);
    ZF_LOGF_IF(err != 0, "Failed to unlock filesystem mutex");
}

/* FAT IO media API */
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

static void mmc_io_flush(
        const uint32_t hard_flush)
{
    if(hard_flush != 0)
    {
        fl_fclose(g_file_handle);

        g_file_handle = fl_fopen(FILE_NAME, "wba");
        ZF_LOGF_IF(g_file_handle == NULL, "Failed to open MMC file '%s'", FILE_NAME);
    }
    else
    {
        const int err = fl_fflush(g_file_handle);
        ZF_LOGF_IF(err != 0, "Failed to flush MMC IO");
    }
}
#endif

static void write_mmc_entry(
        const mmc_entry_s * const entry,
        const uint32_t hard_flush)
{
#ifndef SIMULATION_BUILD
    ZF_LOGF_IF(g_file_handle == NULL, "MMC file handle is invalid");

    const uint16_t entry_size_bytes = entry->size + MMC_ENTRY_HEADER_SIZE;

    int err = fl_fwrite((void*) entry, 1, entry_size_bytes, g_file_handle);
    ZF_LOGF_IF(err != (int) entry_size_bytes, "Failed to write entry to MMC");

    if(hard_flush != 0)
    {
        mmc_io_flush(1);
    }
#endif
}

static int hard_flush_timeout_cb(
        uintptr_t token)
{
#ifndef SIMULATION_BUILD
    if(g_file_handle != NULL)
    {
        mmc_io_flush(1);
    }
#endif

    return 0;

}

static void mmc_thread_fn(const seL4_CPtr ep_cap)
{
    seL4_Word badge;

    /* initialize FAT IO library */
    fl_init();

#ifndef SIMULATION_BUILD
    int err;
    const unsigned long long card_cap = mmc_card_capacity(g_mmc_card);
    MODLOGD("MMC capacity = %llu bytes", card_cap);

    /* attach filesystem mutex functions */
    fl_attach_locks(&fatio_lock, &fatio_unlock);

    /* attach media access functions */
    err = fl_attach_media(&fatio_media_read, &fatio_media_write);
    ZF_LOGF_IF(err != FAT_INIT_OK, "Failed to attach FAT IO media access functions");

    /* open the log file */
    g_file_handle = fl_fopen(FILE_NAME, "wba");
    ZF_LOGF_IF(g_file_handle == NULL, "Failed to open MMC file '%s'", FILE_NAME);
#endif

    time_server_alloc_id(&g_hard_flush_timeout_id);

    time_server_register_periodic_cb(
            MMC_HARD_FLUSH_TIMEOUT_NS,
            0,
            g_hard_flush_timeout_id,
            &hard_flush_timeout_cb,
            0);

    MODLOGD(MMCMOD_THREAD_NAME " thread is running");

    const mmc_entry_s begin_entry_marker =
    {
        .type = MMC_ENTRY_TYPE_BEGIN_FLAG,
        .size = 0
    };

    /* write the begin of log marker */
    write_mmc_entry(&begin_entry_marker, 1);

    /* receive MMC entries via IPC endpoint, write them to the MMC FAT filesystem */
    while(1)
    {
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

        write_mmc_entry(entry, 0);
    }

    if(g_file_handle != NULL)
    {
        fl_fclose(g_file_handle);
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
    ZF_LOGF_IF(err != 0, "Failed to initialize MMC");
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

    /* create a recursive mutex for the FAT IO filesystem */
    const int err = sync_recursive_mutex_new(&env->vka, &g_fs_mutex);
    ZF_LOGF_IF(err != 0, "Failed to MMC filesystem mutex");

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

    MODLOGD("%s is initialized", MMCMOD_THREAD_NAME);

    /* start the new thread */
    thread_start(&g_thread);
}

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
