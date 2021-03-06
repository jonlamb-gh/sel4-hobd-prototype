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

#define COMM_RX_NO_DATA_TIMEOUT_NS MS_TO_NS(400ULL)

typedef enum
{
    COMM_STATE_GPIO_INIT = 0,
    COMM_STATE_SEND_ECU_INIT,
    COMM_STATE_SEND_REQ0,
    COMM_STATE_SEND_REQ1
} comm_state_kind;

typedef struct
{
    uint8_t enabled;
    uint8_t listen_only;
    comm_state_kind state;
    uint32_t timeout_id;
    volatile seL4_Word timeout_signaled;
    ps_chardevice_t char_dev;
    gpio_sys_t gpio_sys;
    gpio_t gpio_uart_tx;
    uint16_t comm_gpio_retry_count;
    uint16_t comm_init_retry_count;
} comm_s;

/* TODO - comm init/reset/etc */
/* TODO - handle errors instead of assert */

uint32_t comm_get_timeout(
        comm_s * const comm);

void comm_stop_timeout(
        comm_s * const comm);

void comm_start_timeout(
        comm_s * const comm);

void comm_gpio_init_seq(
        gpio_t * const gpio,
        comm_s * const comm);

void comm_ecu_init_seq(
        comm_s * const comm);

void comm_send_msg(
        const hobd_msg_s * const msg,
        comm_s * const comm);

hobd_msg_s *comm_recv_msg(
        const uint32_t use_timeout,
        hobd_parser_s * const parser,
        comm_s * const comm);

uint32_t comm_wait_for_resp(
        const uint8_t subtype,
        const uint32_t use_timeout,
        hobd_parser_s * const parser,
        comm_s * const comm,
        uint64_t * const rx_timestamp);

void comm_fill_msg_subgroub_10_query(
        hobd_msg_s * const msg);

void comm_fill_msg_subgroub_d1_query(
        hobd_msg_s * const msg);

#endif /* COMM_H */
