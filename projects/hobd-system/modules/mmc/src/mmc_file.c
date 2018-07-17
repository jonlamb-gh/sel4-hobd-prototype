/**
 * @file mmc_file.c
 * @brief TODO.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <sel4/sel4.h>
#include <sel4debug/debug.h>
#include <allocman/vka.h>
#include <sel4utils/sel4_zf_logif.h>

#include <fat/fat_filelib.h>

#include "config.h"
#include "time_server.h"
#include "mmc_file.h"

#ifdef MMCMOD_DEBUG
#define MODLOGD(...) ZF_LOGD(__VA_ARGS__)
#else
#define MODLOGD(...)
#endif

#define IS_INIT_VALUE (1)

#define DONT_CHECK_FILE (0)
#define CHECK_FILE (1)

/* public symbols */
const char MMC_FILE_NAME[] = "/hobd.log";

static int init_check(
        const mmc_file_s * const file,
        const uint32_t check_file)
{
    int ret = MMC_FILE_ERR_OK;

    ZF_LOGF_IF(file == NULL, "Invalid file parameter");

    if(file->is_init != IS_INIT_VALUE)
    {
        ZF_LOGE("MMC file not initialized");
        ret = MMC_FILE_ERR_INIT;
    }

    if(ret == MMC_FILE_ERR_OK)
    {
        /* only do the check if file is enabled */
        if((check_file != 0) && (file->enabled != 0))
        {
            if(file->ref == NULL)
            {
                ZF_LOGE("MMC file not open");
                ret = MMC_FILE_ERR_NOT_OPEN;
            }
        }
    }

    return ret;
}

static int mmc_fopen(
        mmc_file_s * const file)
{
    int ret = MMC_FILE_ERR_OK;

    file->ref = fl_fopen(MMC_FILE_NAME, "wba");

    if(file->ref == NULL)
    {
        ZF_LOGE("Failed to open MMC file '%s'", MMC_FILE_NAME);
        ret = MMC_FILE_ERR_FS;
    }

    return ret;
}

static void mmc_fclose(
        mmc_file_s * const file)
{
    if(file->ref != NULL)
    {
        fl_fclose(file->ref);
        file->ref = NULL;
    }
}

void mmc_file_init(
        mmc_file_s * const file)
{
    ZF_LOGF_IF(file == NULL, "Invalid file parameter");

    file->ref = NULL;
    file->is_init = IS_INIT_VALUE;

#ifdef SIMULATION_BUILD
    file->enabled = 0;
    MODLOGD("disabling MMC file due to simulation build");
#else
    file->enabled = 1;
#endif
}

int mmc_file_open(
        mmc_file_s * const file)
{
    int ret = init_check(file, DONT_CHECK_FILE);

    if((ret == MMC_FILE_ERR_OK) && (file->enabled != 0))
    {
        if(file->ref == NULL)
        {
            ret = mmc_fopen(file);
        }
    }

    return ret;
}

uint32_t mmc_file_get_enabled(
        const mmc_file_s * const file)
{
    uint32_t enabled = 0;
    const int status = init_check(file, DONT_CHECK_FILE);

    if(status == MMC_FILE_ERR_OK)
    {
        enabled = file->enabled;
    }

    return enabled;
}

int mmc_file_close(
        mmc_file_s * const file)
{
    int ret = init_check(file, DONT_CHECK_FILE);

    if((ret == MMC_FILE_ERR_OK) && (file->enabled != 0))
    {
        mmc_fclose(file);
    }

    return ret;
}

int mmc_file_rm(
        mmc_file_s * const file)
{
    int ret = init_check(file, DONT_CHECK_FILE);

    if((ret == MMC_FILE_ERR_OK) && (file->enabled != 0))
    {
        mmc_fclose(file);

        const int rm_status = fl_remove(MMC_FILE_NAME);

        ret = mmc_fopen(file);

        if(ret == MMC_FILE_ERR_OK)
        {
            if(rm_status != 0)
            {
                ret = MMC_FILE_ERR_FS;
            }
        }
    }

    return ret;
}

int mmc_file_size(
        const mmc_file_s * const file,
        uint32_t * const file_size)
{
    int ret = init_check(file, CHECK_FILE);

    *file_size = 0;

    if((ret == MMC_FILE_ERR_OK) && (file->enabled != 0))
    {
        /* TODO - is this safe? */
        *file_size = (uint32_t) file->ref->filelength;
    }

    return ret;
}

int mmc_file_flush(
        const uint32_t hard_flush,
        mmc_file_s * const file)
{
    int ret = init_check(file, CHECK_FILE);

    if((ret == MMC_FILE_ERR_OK) && (file->enabled != 0))
    {
        if(hard_flush != 0)
        {
            mmc_fclose(file);
            ret = mmc_fopen(file);
        }
        else
        {
            const int flush_status = fl_fflush(file->ref);

            if(flush_status != 0)
            {
                ret = MMC_FILE_ERR_FS;
            }
        }
    }

    return ret;
}

int mmc_file_write(
        const void * const data,
        const uint32_t size,
        const uint32_t count,
        mmc_file_s * const file)
{
    int ret = init_check(file, CHECK_FILE);

    if((ret == MMC_FILE_ERR_OK) && (file->enabled != 0))
    {
        ret = fl_fwrite(
                data,
                size,
                count,
                file->ref);

        if(ret != (int) (size * count))
        {
            ret = MMC_FILE_ERR_IO;
        }
        else
        {
            ret = MMC_FILE_ERR_OK;
        }
    }

    return ret;
}
