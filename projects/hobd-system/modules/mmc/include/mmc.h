/**
 * @file mmc.h
 * @brief TODO.
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

/* TODO
void mmc_log_entry(
        const mmc_entry_s * const entry);
*/

/* not to be called by the MMC thread */
/* length in bytes */
void mmc_log_entry_data(
        const uint16_t type,
        const uint16_t data_size,
        const uint64_t * const timestamp,
        const uint8_t * const data,
        const uint32_t nonblocking);

/* blocking */
void mmc_request_stats(
        mmc_stats_s * const stats);

#endif /* MMC_H */
