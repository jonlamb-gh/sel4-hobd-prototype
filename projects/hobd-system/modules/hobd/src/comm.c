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
#include <platsupport/gpio.h>
#include <platsupport/ltimer.h>
#include <sel4debug/debug.h>

#include "config.h"
#include "hobd_kline.h"
#include "hobd_parser.h"
#include "hobd_msg.h"
#include "comm.h"

void comm_timestamp(
    uint64_t * const time,
    comm_s * const comm)
{
    const int err = ltimer_get_time(&comm->timer, time);
    ZF_LOGF_IF(err != 0, "Failed to get time\n");
}

void comm_gpio_init_seq(
        gpio_t * const gpio,
        comm_s * const comm)
{
    int err;

    /* pull k-line low for 70 ms */
    err = gpio_clr(gpio);
    ZF_LOGF_IF(err != 0, "Failed to clear k-line GPIO\n");

    ps_mdelay(70);

    /* return to high, wait 120 ms */
    err = gpio_set(gpio);
    ZF_LOGF_IF(err != 0, "Failed to set k-line GPIO\n");

    ps_mdelay(120);
}

void comm_send_msg(
        const hobd_msg_s * const msg,
        comm_s * const comm)
{
    uint16_t idx;

    for(idx = 0; idx < msg->header.size; idx += 1)
    {
        ps_cdev_putchar(
                &comm->char_dev,
                ((uint8_t*) msg)[idx]);
    }

    ZF_LOGD(
            "tx_msg[%02X:%02X:%02X]",
            (unsigned int) msg->header.type,
            (unsigned int) msg->header.size,
            (unsigned int) msg->header.subtype);
}

/* TODO - timeouts ? */
hobd_msg_s *comm_recv_msg(
        hobd_parser_s * const parser,
        comm_s * const comm)
{
    hobd_msg_s *msg = NULL;

    while(msg == NULL)
    {
        const int data = ps_cdev_getchar(&comm->char_dev);

        if(data >= 0)
        {
            //ZF_LOGD("got data: 0x%02X", (unsigned int) data);

            const uint8_t status = hobd_parser_parse_byte(
                    (uint8_t) data,
                    parser);

            if(status == HOBD_ERROR_NONE)
            {
                msg = (hobd_msg_s*) &parser->rx_buffer[0];

                ZF_LOGD(
                    "rx_msg[%02X:%02X:%02X]",
                    (unsigned int) msg->header.type,
                    (unsigned int) msg->header.size,
                    (unsigned int) msg->header.subtype);
            }
        }
    }

    return msg;
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
    //query->count = 0x80;

    (void) hobd_msg(
            HOBD_MSG_TYPE_QUERY,
            HOBD_MSG_SUBTYPE_TABLE_SUBGROUP,
            (uint8_t) sizeof(query),
            (uint8_t*) msg);
}
