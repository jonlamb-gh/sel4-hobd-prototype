/**
 * @file hobd.h
 * @brief TODO.
 *
 */

#ifndef HOBD_H
#define HOBD_H

#include <stdint.h>

typedef struct
{
    uint64_t timestamp;
    uint16_t valid_rx_count;
    uint16_t invalid_rx_count;
    uint16_t comm_gpio_retry_count;
    uint16_t comm_init_retry_count;
} __attribute__((__packed__)) hobd_stats_s;

/* these are all blocking, they use seL4_Call() */
void hobd_get_stats(
        hobd_stats_s * const stats);

uint32_t hobd_set_comm_state(
        const uint32_t state);

uint32_t hobd_get_comm_state(void);

#endif /* HOBD_H */
