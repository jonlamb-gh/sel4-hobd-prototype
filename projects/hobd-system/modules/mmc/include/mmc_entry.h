/**
 * @file mmc_entry.h
 * @brief TODO.
 *
 */

#ifndef MMC_ENTRY_H
#define MMC_ENTRY_H

#include <stdint.h>

#define MMC_ENTRY_HEADER_SIZE (4)

/* bytes(seL4_MsgMaxLength) - sizeof(header) */
#define MMC_ENTRY_DATA_SIZE_MAX ((120 * 4) - MMC_ENTRY_HEADER_SIZE)

#define MMC_ENTRY_TYPE_INVALID (0x0000)
#define MMC_ENTRY_TYPE_BEGIN_FLAG (0xABFE)
#define MMC_ENTRY_TYPE_LOG_MSG (0x00A0)
#define MMC_ENTRY_TYPE_HOBD_MSG (0x0100)

/* TODO - should size be total size? */
typedef struct
{
    uint16_t type;
    uint16_t size; // size of data[] in bytes
    uint8_t data[0];
} __attribute__((__packed__)) mmc_entry_s;

#endif /* MMC_ENTRY_H */
