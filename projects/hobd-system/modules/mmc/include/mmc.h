/**
 * @file mmc.h
 * @brief TODO.
 *
 * @note *NOT* to be called by the internal MMC module thread.
 *
 */

#ifndef MMC_H
#define MMC_H

#include <stdint.h>

#include "mmc_entry.h"

typedef struct
{
    uint64_t timestamp;
    uint32_t entries_logged;
} __attribute__((__packed__)) mmc_stats_s;

/* TODO ?
void mmc_log_entry(
        const mmc_entry_s * const entry);
*/

/* length in bytes */
void mmc_log_entry_data(
        const uint16_t type,
        const uint16_t data_size,
        const uint64_t * const timestamp,
        const uint8_t * const data,
        const uint32_t nonblocking);

/* these are all blocking, they use seL4_Call() */
void mmc_get_stats(
        mmc_stats_s * const stats);

/* 0 = success */
int mmc_rm(void);

/* 0 = success */
int mmc_get_file_size(
        uint32_t * const size);

#endif /* MMC_H */
