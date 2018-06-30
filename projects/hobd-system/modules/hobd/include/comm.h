/**
 * @file comm.h
 * @brief TODO.
 *
 */

#ifndef COMM_H
#define	COMM_H

#include <stdint.h>

#include <platsupport/chardev.h>
#include <platsupport/gpio.h>
#include <platsupport/ltimer.h>

#include "hobd_parser.h"
#include "hobd_kline.h"

typedef struct
{
    ps_chardevice_t char_dev;
    gpio_sys_t gpio_sys;
    ltimer_t timer;
} comm_s;

/* TODO - comm init/reset/etc */
/* TODO - probably should handle errors instead of assert */

/* nanoseconds */
void comm_timestamp(
    uint64_t * const time,
    comm_s * const comm);

void comm_gpio_init_seq(
        gpio_t * const gpio,
        comm_s * const comm);

void comm_send_msg(
        const hobd_msg_s * const msg,
        comm_s * const comm);

/* TODO - timeouts ? */
hobd_msg_s *comm_recv_msg(
        hobd_parser_s * const parser,
        comm_s * const comm);

void comm_fill_msg_subgroub_10_query(
        hobd_msg_s * const msg);

#endif /* COMM_H */
