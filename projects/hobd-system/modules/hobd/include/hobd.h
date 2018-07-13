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

/* blocking */
void hobd_request_stats(
        hobd_stats_s * const stats);

#endif /* HOBD_H */
