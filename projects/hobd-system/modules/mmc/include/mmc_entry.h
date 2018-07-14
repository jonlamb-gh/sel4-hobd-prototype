/**
 * @file mmc_entry.h
 * @brief TODO.
 *
 * TODO:
 * - should size be total size?
 * - add fault type
 * - add prev_size for file seeking?
 *
 */

#ifndef MMC_ENTRY_H
#define MMC_ENTRY_H

#include <stdint.h>

#define MMC_ENTRY_HEADER_SIZE (12)

/* bytes(seL4_MsgMaxLength) - sizeof(header) */
#define MMC_ENTRY_SIZE_MIN MMC_ENTRY_HEADER_SIZE
#define MMC_ENTRY_SIZE_MAX (120 * 4)
#define MMC_ENTRY_DATA_SIZE_MAX (MMC_ENTRY_SIZE_MAX - MMC_ENTRY_HEADER_SIZE)

#define MMC_ENTRY_TYPE_INVALID (0x0000)
#define MMC_ENTRY_TYPE_BEGIN_FLAG (0xABFE)
#define MMC_ENTRY_TYPE_FAULT (0x0010)
#define MMC_ENTRY_TYPE_HEARTBEAT (0x0020)
#define MMC_ENTRY_TYPE_HOBD_MSG (0x0100)

typedef struct
{
    uint16_t type;
    uint16_t size; // size of data[] in bytes
    uint64_t timestamp;
} __attribute__((__packed__)) mmc_entry_header_s;

typedef struct
{
    mmc_entry_header_s header;
    uint8_t data[0];
} __attribute__((__packed__)) mmc_entry_s;

static inline uint16_t mmc_entry_total_size(
        const mmc_entry_s * const entry)
{
    return entry->header.size + MMC_ENTRY_HEADER_SIZE;
}

#endif /* MMC_ENTRY_H */
