/**
 * @file comm.c
 * @brief TODO.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <autoconf.h>
#include <sel4/sel4.h>
#include <platsupport/delay.h>
#include <platsupport/io.h>
#include <platsupport/chardev.h>
#include <platsupport/serial.h>
#include <platsupport/gpio.h>
#include <platsupport/ltimer.h>
#include <sel4debug/debug.h>

#include "config.h"
#include "time_server.h"
#include "hobd_kline.h"
#include "hobd_parser.h"
#include "hobd_msg.h"
#include "comm.h"

#ifdef HOBDMOD_DEBUG
#define MODLOGD(...) ZF_LOGD(__VA_ARGS__)
#else
#define MODLOGD(...)
#endif

static int timeout_cb(
        uintptr_t token)
{
    ZF_LOGF_IF(token == 0, "Invalid token, expexted comm_s pointer");

    /* TODO - is this atomic? do I need __sync_...? */
    comm_s * const comm = (comm_s*) token;
    comm->timeout_signaled = 1;

    return 0;

}

uint32_t comm_get_timeout(
        comm_s * const comm)
{
    return (uint32_t) comm->timeout_signaled;
}

void comm_stop_timeout(
        comm_s * const comm)
{
    time_server_deregister_cb(comm->timeout_id);
    comm->timeout_signaled = 0;
}

void comm_start_timeout(
        comm_s * const comm)
{
    comm_stop_timeout(comm);

    time_server_register_rel_cb(
            COMM_RX_NO_DATA_TIMEOUT_NS,
            comm->timeout_id,
            &timeout_cb,
            (uintptr_t) comm);
}

void comm_gpio_init_seq(
        gpio_t * const gpio,
        comm_s * const comm)
{
    int err;

    /* pull k-line low for 70 ms */
    err = gpio_clr(gpio);
    ZF_LOGF_IF(err != 0, "Failed to clear k-line GPIO");

    ps_mdelay(70);

    /* return to high, wait 120 ms */
    err = gpio_set(gpio);
    ZF_LOGF_IF(err != 0, "Failed to set k-line GPIO");

    ps_mdelay(120);
}

void comm_ecu_init_seq(
        comm_s * const comm)
{
    MODLOGD("Performing ECU GPIO initialization sequence");

    /* perform the init sequence */
    comm_gpio_init_seq(&comm->gpio_uart_tx, comm);

    /* reconfigure the serial port */
    const int err = serial_configure(
            &comm->char_dev,
            HOBD_KLINE_BAUD,
            8,
            PARITY_NONE,
            1);
    ZF_LOGF_IF(err != 0, "Failed to configure serial port");
}

void comm_send_msg(
        const hobd_msg_s * const msg,
        comm_s * const comm)
{
    uint16_t idx;

    if(comm->listen_only == 0)
    {
        for(idx = 0; idx < msg->header.size; idx += 1)
        {
            ps_cdev_putchar(
                    &comm->char_dev,
                    ((uint8_t*) msg)[idx]);
        }

        MODLOGD(
                "tx_msg[%02X:%02X:%02X]",
                (unsigned int) msg->header.type,
                (unsigned int) msg->header.size,
                (unsigned int) msg->header.subtype);
    }
}

hobd_msg_s *comm_recv_msg(
        const uint32_t use_timeout,
        hobd_parser_s * const parser,
        comm_s * const comm)
{
    hobd_msg_s *msg = NULL;
    uint32_t timeout_fired = 0;

    if(use_timeout != 0)
    {
        comm_start_timeout(comm);
    }

    while((msg == NULL) && (timeout_fired == 0))
    {
        const int data = ps_cdev_getchar(&comm->char_dev);

        if(data >= 0)
        {
            //MODLOGD("got data: 0x%02X", (unsigned int) data);

            const uint8_t status = hobd_parser_parse_byte(
                    (uint8_t) data,
                    parser);

            if(status == HOBD_ERROR_NONE)
            {
                msg = (hobd_msg_s*) &parser->rx_buffer[0];

                MODLOGD(
                    "rx_msg[%02X:%02X:%02X]",
                    (unsigned int) msg->header.type,
                    (unsigned int) msg->header.size,
                    (unsigned int) msg->header.subtype);
            }
        }

        if(use_timeout != 0)
        {
            timeout_fired = comm_get_timeout(comm);
        }
    }

    if(use_timeout != 0)
    {
        comm_stop_timeout(comm);
    }

    return msg;
}

uint32_t comm_wait_for_resp(
        const uint8_t subtype,
        const uint32_t use_timeout,
        hobd_parser_s * const parser,
        comm_s * const comm,
        uint64_t * const rx_timestamp)
{
    uint32_t resp_found = 0;
    uint32_t timeout_fired = 0;

    while((resp_found == 0) && (timeout_fired == 0))
    {
        const hobd_msg_s * const msg = comm_recv_msg(
                use_timeout,
                parser,
                comm);

        if(msg != NULL)
        {
            if(msg->header.type == HOBD_MSG_TYPE_RESPONSE)
            {
                if(msg->header.subtype == subtype)
                {
                    resp_found = 1;
                }
            }
        }
        else
        {
            timeout_fired = 1;
        }
    }

    if((resp_found != 0) && (rx_timestamp != NULL))
    {
        time_server_get_time(rx_timestamp);
        MODLOGD("Response msg time is %llu ns", *rx_timestamp);
    }

    return resp_found;
}

void comm_fill_msg_subgroub_10_query(
        hobd_msg_s * const msg)
{
    hobd_data_table_query_s * const query =
            (hobd_data_table_query_s*) &msg->data[0];

    /* TODO */
    query->table = HOBD_TABLE_10;
    query->offset = 0;
    query->count = 0x80;

    (void) hobd_msg(
            HOBD_MSG_TYPE_QUERY,
            HOBD_MSG_SUBTYPE_TABLE_SUBGROUP,
            (uint8_t) sizeof(query),
            (uint8_t*) msg);
}

void comm_fill_msg_subgroub_d1_query(
        hobd_msg_s * const msg)
{
    hobd_data_table_query_s * const query =
            (hobd_data_table_query_s*) &msg->data[0];

    /* TODO */
    query->table = HOBD_TABLE_D1;
    query->offset = 0;
    query->count = 0x06;

    (void) hobd_msg(
            HOBD_MSG_TYPE_QUERY,
            HOBD_MSG_SUBTYPE_TABLE_SUBGROUP,
            (uint8_t) sizeof(query),
            (uint8_t*) msg);
}
