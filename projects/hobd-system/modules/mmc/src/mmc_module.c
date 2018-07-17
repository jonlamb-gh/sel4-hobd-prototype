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
#include "ipc_util.h"
#include "init_env.h"
#include "thread.h"
#include "sel4_mr.h"
#include "time_server.h"
#include "mmc_file.h"
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

#define ENDPOINT_BADGE IPC_ENDPOINT_BADGE(MMCMOD_BASE_BADGE)

#define IPC_MSG_TYPE_ENTRY_LOG IPC_MSG_TYPE_ID(MMCMOD_BASE_BADGE, 1)
#define IPC_MSG_TYPE_STATS_REQ IPC_MSG_TYPE_ID(MMCMOD_BASE_BADGE, 2)
#define IPC_MSG_TYPE_STATS_RESP IPC_MSG_TYPE_ID(MMCMOD_BASE_BADGE, 3)
#define IPC_MSG_TYPE_RM_FILE_REQ IPC_MSG_TYPE_ID(MMCMOD_BASE_BADGE, 4)
#define IPC_MSG_TYPE_RM_FILE_RESP IPC_MSG_TYPE_ID(MMCMOD_BASE_BADGE, 5)
#define IPC_MSG_TYPE_FILE_SIZE_REQ IPC_MSG_TYPE_ID(MMCMOD_BASE_BADGE, 6)
#define IPC_MSG_TYPE_FILE_SIZE_RESP IPC_MSG_TYPE_ID(MMCMOD_BASE_BADGE, 7)

static sdio_host_dev_t g_sdio_dev;
static mmc_card_t g_mmc_card;
static mmc_file_s g_mmc_file;

static sync_recursive_mutex_t g_fs_mutex;
static uint32_t g_hard_flush_timeout_id;

static seL4_Word g_msg_rx_buffer[seL4_MsgMaxLength];

static thread_s g_thread;
static uint64_t g_thread_stack[MMCMOD_STACK_SIZE];

/* Align 'n' up to the value 'align', which must be a power of two. */
static seL4_Word align_up(
        const seL4_Word n,
        const seL4_Word align)
{
    return (n + align - 1) & (~(align - 1));
}

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
    const uint32_t enabled = mmc_file_get_enabled(&g_mmc_file);

    if(enabled != 0)
    {
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
    const uint32_t enabled = mmc_file_get_enabled(&g_mmc_file);

    if(enabled != 0)
    {
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
    }

    return ret;
}

static void write_mmc_entry(
        const mmc_entry_s * const entry,
        const uint32_t hard_flush)
{
    const uint16_t entry_size_bytes = mmc_entry_total_size(entry);

    const int err = mmc_file_write(
            (void*) entry,
            1,
            entry_size_bytes,
            &g_mmc_file);
    ZF_LOGF_IF(err != 0, "Failed to write entry to MMC");

    if(hard_flush != 0)
    {
        mmc_file_flush(1, &g_mmc_file);
    }
}

static int hard_flush_timeout_cb(
        uintptr_t token)
{
    mmc_file_flush(1, &g_mmc_file);

    return 0;
}

static void mmc_thread_fn(
        const seL4_CPtr ep_cap)
{
    int err;
    seL4_Word badge;
    mmc_stats_s stats = {0};

    mmc_file_init(&g_mmc_file);

    /* initialize FAT IO library */
    fl_init();

#ifndef SIMULATION_BUILD
    MODLOGD("MMC capacity = %llu bytes", mmc_card_capacity(g_mmc_card));
#endif

    /* attach filesystem mutex functions */
    fl_attach_locks(&fatio_lock, &fatio_unlock);

    /* attach media access functions */
    err = fl_attach_media(&fatio_media_read, &fatio_media_write);

#ifdef SIMULATION_BUILD
    ZF_LOGW_IF(
            err != FAT_INIT_OK,
            "Failed to attach FAT IO media access functions - "
            "this is expected due to simulation build");
#else
    ZF_LOGF_IF(err != FAT_INIT_OK, "Failed to attach FAT IO media access functions");
#endif

    MODLOGD(MMCMOD_THREAD_NAME " thread is running");

    /* open the log file */
    err = mmc_file_open(&g_mmc_file);
    ZF_LOGF_IF(err != 0, "Handling of MMC errors at runtime not yet implemented");

    time_server_alloc_id(&g_hard_flush_timeout_id);

    time_server_register_periodic_cb(
            MMC_HARD_FLUSH_TIMEOUT_NS,
            0,
            g_hard_flush_timeout_id,
            &hard_flush_timeout_cb,
            0);

    time_server_get_time(&stats.timestamp);

    const mmc_entry_s begin_entry_marker =
    {
        .header =
        {
            .type = MMC_ENTRY_TYPE_BEGIN_FLAG,
            .size = 0,
            .timestamp = stats.timestamp,
        }
    };

    /* write the begin of log marker */
    write_mmc_entry(&begin_entry_marker, 1);
    stats.entries_logged += 1;

    /* receive MMC entries via IPC endpoint, write them to the MMC FAT filesystem */
    while(1)
    {
        const seL4_MessageInfo_t info = seL4_Recv(
                ep_cap,
                &badge);

        ZF_LOGF_IF(badge != ENDPOINT_BADGE, "Invalid IPC badge");

        const seL4_Word msg_label = seL4_MessageInfo_get_label(info);

        if(msg_label == IPC_MSG_TYPE_STATS_REQ)
        {
            time_server_get_time(&stats.timestamp);

            const uint32_t resp_size_words = sizeof(stats) / sizeof(seL4_Word);

            const seL4_MessageInfo_t resp_info =
                    seL4_MessageInfo_new(
                        IPC_MSG_TYPE_STATS_RESP,
                        0,
                        0,
                        resp_size_words);

            sel4_mr_send(resp_size_words, (uint32_t*) &stats, g_thread.ipc_buffer);

            seL4_Reply(resp_info);
        }
        else if(msg_label == IPC_MSG_TYPE_ENTRY_LOG)
        {
            const seL4_Word total_size_words = seL4_MessageInfo_get_length(info);

            sel4_mr_recv(g_thread.ipc_buffer, total_size_words, (uint32_t*) &g_msg_rx_buffer[0]);

            const mmc_entry_s * const entry =
                    (const mmc_entry_s*) &g_msg_rx_buffer[0];

            ZF_LOGF_IF(entry->header.type == MMC_ENTRY_TYPE_INVALID, "Invalid entry type");
            ZF_LOGF_IF(entry->header.size > MMC_ENTRY_DATA_SIZE_MAX, "Entry size is too large");

            write_mmc_entry(entry, 0);

            stats.entries_logged += 1;
        }
        else if(msg_label == IPC_MSG_TYPE_FILE_SIZE_REQ)
        {
            uint32_t file_size = 0;

            const uint32_t cmd_status = (uint32_t) mmc_file_size(&g_mmc_file, &file_size);

            const seL4_MessageInfo_t resp_info =
                    seL4_MessageInfo_new(IPC_MSG_TYPE_FILE_SIZE_RESP, 0, 0, 2);

            seL4_SetMR(0, (seL4_Word) cmd_status);
            seL4_SetMR(1, (seL4_Word) file_size);

            seL4_Reply(resp_info);
        }
        else if(msg_label == IPC_MSG_TYPE_RM_FILE_REQ)
        {
            stats.entries_logged = 0;

            const uint32_t cmd_status = (uint32_t) mmc_file_rm(&g_mmc_file);

            const seL4_MessageInfo_t resp_info =
                    seL4_MessageInfo_new(IPC_MSG_TYPE_RM_FILE_RESP, 0, 0, 1);

            seL4_SetMR(0, (seL4_Word) cmd_status);

            seL4_Reply(resp_info);
        }
        else
        {
            ZF_LOGF("Invalid message label");
        }
    }

    (void) mmc_file_close(&g_mmc_file);

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

#ifdef SIMULATION_BUILD
    MODLOGD("skipping mmc_init() due to simulation build");
#else
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
            ENDPOINT_BADGE,
            (uint32_t) sizeof(g_thread_stack),
            &g_thread_stack[0],
            &mmc_thread_fn,
            env,
            &g_thread);

    /* set thread priority and affinity */
    thread_set_priority(MMCMOD_THREAD_PRIORITY, &g_thread);
    thread_set_affinity(MMCMOD_THREAD_AFFINITY, &g_thread);

    MODLOGD("%s is initialized", MMCMOD_THREAD_NAME);

    /* start the new thread */
    thread_start(&g_thread);
}

void mmc_log_entry_data(
        const uint16_t type,
        const uint16_t data_size,
        const uint64_t * const timestamp,
        const uint8_t * const data,
        const uint32_t nonblocking)
{
    uint64_t local_timestamp = 0;
    const uint64_t *tstamp_ref = &local_timestamp;

    if(timestamp != NULL)
    {
        tstamp_ref = timestamp;
    }
    else
    {
        time_server_get_time(&local_timestamp);
    }

    const seL4_Word size_bytes = (seL4_Word) (data_size + MMC_ENTRY_HEADER_SIZE);
    const seL4_Word total_size_bytes = align_up(size_bytes, 4);
    const seL4_Word total_size_words = (seL4_Word) (total_size_bytes / sizeof(seL4_Word));

    ZF_LOGF_IF((total_size_bytes % 4) != 0, "Invalid MMC entry data alignment");

    ZF_LOGF_IF(
            total_size_bytes > MMC_ENTRY_DATA_SIZE_MAX,
            "Failed to log MMC entry data - data size too large");

    const seL4_MessageInfo_t info =
            seL4_MessageInfo_new(IPC_MSG_TYPE_ENTRY_LOG, 0, 0, total_size_words);

    /* header in R0:R2 */
    seL4_SetMR(0, (seL4_Word) (type | (data_size << 16)));
    seL4_SetMR(1, (seL4_Word) ((*tstamp_ref) & 0xFFFFFFFF));
    seL4_SetMR(2, (seL4_Word) (((*tstamp_ref) >> 32) & 0xFFFFFFFF));

    /* -3 for header size words */
    uint32_t idx;
    for(idx = 0; idx < (total_size_words - 3); idx += 1)
    {
        seL4_Word data_word = 0;

        uint32_t shift;
        for(shift = 0; shift < 4; shift += 1)
        {
            const uint32_t data_index = (idx * 4) + shift;

            if(data_index < (uint32_t) data_size)
            {
                data_word |= (data[data_index] << (shift * 8));
            }
        }

        /* TODO - what am I doing wrong here to require this? */
        /* use R3, then remaining go into the IPC buffer msg array */
        if(idx == 0)
        {
            seL4_SetMR(3 + idx, data_word);
        }
        else
        {
            g_thread.ipc_buffer->msg[3 + idx] = data_word;
        }
    }

    if(nonblocking == 0)
    {
        seL4_Send(g_thread.ipc_ep_cap, info);
    }
    else
    {
        seL4_NBSend(g_thread.ipc_ep_cap, info);
    }
}

void mmc_get_stats(
        mmc_stats_s * const stats)
{
    ZF_LOGF_IF(sizeof(stats) % sizeof(seL4_Word) != 0, "Stats struct not properly aligned");

    const seL4_MessageInfo_t req_info =
            seL4_MessageInfo_new(IPC_MSG_TYPE_STATS_REQ, 0, 0, 0);

    const seL4_MessageInfo_t resp_info = seL4_Call(g_thread.ipc_ep_cap, req_info);

    ZF_LOGF_IF(
            seL4_MessageInfo_get_label(resp_info) != IPC_MSG_TYPE_STATS_RESP,
            "Invalid response lable");

    const uint32_t resp_size_words = sizeof(*stats) / sizeof(seL4_Word);

    ZF_LOGF_IF(
            (seL4_MessageInfo_get_length(resp_info) != resp_size_words),
            "Invalid response length");

    sel4_mr_recv(g_thread.ipc_buffer, resp_size_words, (uint32_t*) stats);
}

int mmc_rm(void)
{
    int ret = 0;

    const seL4_MessageInfo_t req_info =
            seL4_MessageInfo_new(IPC_MSG_TYPE_RM_FILE_REQ, 0, 0, 0);

    const seL4_MessageInfo_t resp_info = seL4_Call(g_thread.ipc_ep_cap, req_info);

    ZF_LOGF_IF(
            seL4_MessageInfo_get_label(resp_info) != IPC_MSG_TYPE_RM_FILE_RESP,
            "Invalid response lable");

    ZF_LOGF_IF(
            seL4_MessageInfo_get_length(resp_info) != 1,
            "Invalid response length");

    if(ret == 0)
    {
        const seL4_Word cmd_resp = seL4_GetMR(0);

        ret = (int) cmd_resp;
    }

    return ret;
}

int mmc_get_file_size(
        uint32_t * const size)
{
    int ret = 0;

    const seL4_MessageInfo_t req_info =
            seL4_MessageInfo_new(IPC_MSG_TYPE_FILE_SIZE_REQ, 0, 0, 0);

    const seL4_MessageInfo_t resp_info = seL4_Call(g_thread.ipc_ep_cap, req_info);

    ZF_LOGF_IF(
            seL4_MessageInfo_get_label(resp_info) != IPC_MSG_TYPE_FILE_SIZE_RESP,
            "Invalid response lable");

    ZF_LOGF_IF(
            seL4_MessageInfo_get_length(resp_info) != 2,
            "Invalid response length");

    if(ret == 0)
    {
        const seL4_Word cmd_resp = seL4_GetMR(0);
        const seL4_Word file_size = seL4_GetMR(1);

        ret = (int) cmd_resp;

        if(ret == 0)
        {
            *size = (uint32_t) file_size;
        }
        else
        {
            *size = 0;
        }
    }

    return ret;
}
