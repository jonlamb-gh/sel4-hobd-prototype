/**
 * @file mmc.h
 * @brief TODO.
 *
 */

#ifndef MMC_H
#define MMC_H

#include <stdint.h>

#include <sel4/bootinfo_types.h>

#include "mmc_entry.h"

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

#endif /* MMC_H */
