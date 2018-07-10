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
#include "mmc.h"
#include "mmc_module.h"

#define FATIO_MEDIA_RW_FAILURE (0)
#define FATIO_MEDIA_RW_SUCCESS (1)

static sdio_host_dev_t g_sdio_dev;
static mmc_card_t g_mmc_card;

static thread_s g_thread;
static uint64_t g_thread_stack[MMCMOD_STACK_SIZE];

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
    int err;
    seL4_Word badge;

    /* initialize FAT IO library */
    fl_init();

#ifndef SIMULATION_BUILD
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
        (void) err;

        const seL4_MessageInfo_t info = seL4_Recv(
                ep_cap,
                &badge);

        (void) info;

        ZF_LOGD(
                "mmc IPC msg badge = 0x%X - label = 0x%X - length = %u",
                badge,
                seL4_MessageInfo_get_label(info),
                seL4_MessageInfo_get_length(info));

        /*
        FL_FILE * const file = fl_fopen("/file.txt", "wa");

        if(file != NULL)
        {
            const char str[] = "hello\n";

            err = fl_fputs(str, file);
            ZF_LOGF_IF(err != (sizeof(str) - 1), "Failed to write to MMC card");

            fl_fclose(file);
        }
        else
        {
            ZF_LOGW("failed to open MMC card file");
        }
        ps_sdelay(1);
        */
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

seL4_CPtr mmc_get_ipc_cap(void)
{
    return g_thread.ipc_ep_cap;
}
