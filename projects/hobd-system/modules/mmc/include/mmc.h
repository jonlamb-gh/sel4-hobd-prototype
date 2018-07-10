/**
 * @file mmc.h
 * @brief TODO.
 *
 */

#ifndef MMC_H
#define MMC_H

#include <stdint.h>

#include <sel4/bootinfo_types.h>

/* TODO - mmc_protocol.h */

/* TODO - not thread safe */

seL4_CPtr mmc_get_ipc_cap(void);

#endif /* MMC_H */
