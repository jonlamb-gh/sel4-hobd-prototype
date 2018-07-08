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

#include "hobd_parser.h"
#include "hobd_kline.h"

typedef enum
{
    COMM_STATE_GPIO_INIT = 0,
    COMM_STATE_SEND_ECU_INIT,
    COMM_STATE_SEND_REQ0,
    COMM_STATE_SEND_REQ1
} comm_state_kind;

typedef struct
{
    comm_state_kind state;
    ps_chardevice_t char_dev;
    gpio_sys_t gpio_sys;
    gpio_t gpio_uart_tx;
} comm_s;

/* TODO - comm init/reset/etc */
/* TODO - handle errors instead of assert */

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

void comm_fill_msg_subgroub_d1_query(
        hobd_msg_s * const msg);

#endif /* COMM_H */
