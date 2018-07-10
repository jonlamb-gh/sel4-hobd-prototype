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

/* TODO - not thread safe */

/* not to be called by the MMC thread */

/* length in bytes */
//void mmc_log_entry(const ... entry);
void mmc_log_entry_data(
        const uint16_t type,
        const uint16_t data_size,
        const uint8_t * const data);

#endif /* MMC_H */
