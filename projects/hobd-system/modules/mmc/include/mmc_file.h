/**
 * @file mmc_file.h
 * @brief TODO.
 *
 */

#ifndef MMC_FILE_H
#define MMC_FILE_H

#include <stdint.h>

#include <fat/fat_filelib.h>

#define MMC_FILE_ERR_OK (0)
#define MMC_FILE_ERR_INIT (1)
#define MMC_FILE_ERR_FS (2)
#define MMC_FILE_ERR_NOT_OPEN (3)
#define MMC_FILE_ERR_IO (4)

typedef struct
{
    uint32_t is_init;
    uint32_t enabled;
    FL_FILE *ref;
} mmc_file_s;

extern const char MMC_FILE_NAME[];

void mmc_file_init(
        mmc_file_s * const file);

uint32_t mmc_file_get_enabled(
        const mmc_file_s * const file);

int mmc_file_open(
        mmc_file_s * const file);

int mmc_file_close(
        mmc_file_s * const file);

int mmc_file_rm(
        mmc_file_s * const file);

int mmc_file_size(
        const mmc_file_s * const file,
        uint32_t * const file_size);

int mmc_file_flush(
        const uint32_t hard_flush,
        mmc_file_s * const file);

int mmc_file_write(
        const void * const data,
        const uint32_t size,
        const uint32_t count,
        mmc_file_s * const file);

#endif /* MMC_FILE_H */
