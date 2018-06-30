/**
 * @file ecu.c
 * @brief TODO.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>

#include "hobd_kline.h"
#include "hobd_parser.h"
#include "ecu.h"

typedef enum
{
    STATE_WAIT4_WAKEUP = 0,
    STATE_WAIT4_DIAG_INIT,
    STATE_ACTIVE_LISTEN
} ecu_state_kind;

struct ecu_s
{
    int socket_fd;
    ecu_state_kind state;
    hobd_parser_s rx_parser;
    uint8_t table_0_page[HOBD_TABLE_SIZE_MAX];
    uint8_t table_10_page[HOBD_TABLE_SIZE_MAX];
    uint8_t table_20_page[HOBD_TABLE_SIZE_MAX];
    uint8_t table_D1_page[HOBD_TABLE_SIZE_MAX];
    uint8_t rx_buffer[HOBD_MSG_SIZE_MAX + HOBD_MSG_HEADERCS_SIZE];
    uint8_t tx_buffer[HOBD_MSG_SIZE_MAX + HOBD_MSG_HEADERCS_SIZE];
};

static int check_for_data(
        ecu_s * const ecu,
        uint8_t * const data)
{
    int num_bytes = recv(
            ecu->socket_fd,
            data,
            1,
            MSG_DONTWAIT);

    if(num_bytes < 0)
    {
        if((errno == EAGAIN) || (errno == EWOULDBLOCK))
        {
            num_bytes = 0;
        }
        else
        {
            fprintf(stderr, "call to recv failed\n");
        }
    }

    return num_bytes;
}

static uint8_t check_for_message(
        ecu_s * const ecu)
{
    uint8_t msg_type = HOBD_MSG_TYPE_INVALID;
    uint8_t data;

    const int num_bytes = check_for_data(ecu, &data);

    if(num_bytes > 0)
    {
        const uint8_t status = hobd_parser_parse_byte(
                data,
                &ecu->rx_parser);

        if(status == HOBD_ERROR_NONE)
        {
            const hobd_msg_header_s * const header =
                    (hobd_msg_header_s*) ecu->rx_buffer;

            msg_type = header->type;

            /* TESTING */
            printf(
                    "rx_msg[%02X:%02X:%02X]\n",
                    (unsigned int) header->type,
                    (unsigned int) header->size,
                    (unsigned int) header->subtype);
        }
    }

    return msg_type;
}

static int send_message(
        const hobd_msg_s * const msg,
        ecu_s * const ecu)
{
    int bytes_written = 0;

    /* TESTING */
    printf(
            "tx_msg[%02X:%02X:%02X]\n",
            (unsigned int) msg->header.type,
            (unsigned int) msg->header.size,
            (unsigned int) msg->header.subtype);

    const ssize_t num_bytes = send(
            ecu->socket_fd,
            (const void*) msg,
            (size_t) msg->header.size,
            0);

    if(num_bytes != (ssize_t) msg->header.size)
    {
        fprintf(stderr, "call to send failed\n");
    }
    else
    {
        bytes_written = (int) msg->header.size;
    }

    return bytes_written;
}

static uint8_t *get_data_table(
        const uint8_t table,
        ecu_s * const ecu)
{
    uint8_t *src_table = NULL;

    if(table == HOBD_TABLE_0)
    {
        src_table = &ecu->table_0_page[0];
    }
    else if(table == HOBD_TABLE_10)
    {
        src_table = &ecu->table_10_page[0];
    }
    else if(table == HOBD_TABLE_20)
    {
        src_table = &ecu->table_20_page[0];
    }
    else if(table == HOBD_TABLE_D1)
    {
        src_table = &ecu->table_D1_page[0];
    }

    return src_table;
}

static void handle_rx_message(
        const hobd_msg_s * const msg,
        ecu_s * const ecu)
{
    /* TODO - behavior for invalid data/etc */
    hobd_msg_s * const tx_msg =
            (hobd_msg_s*) &ecu->tx_buffer[0];

    /* handle OBD init message response */
    if(
            (msg->header.type == HOBD_MSG_TYPE_QUERY)
            && (msg->header.subtype == HOBD_MSG_SUBTYPE_INIT))
    {
        tx_msg->header.type = HOBD_MSG_TYPE_RESPONSE;
        tx_msg->header.size = HOBD_MSG_HEADERCS_SIZE;
        tx_msg->header.subtype = HOBD_MSG_SUBTYPE_INIT;

        /* TODO - check init data HOBD_INIT_DATA */

        tx_msg->data[0] = hobd_parser_checksum(
                (uint8_t*) tx_msg,
                tx_msg->header.size - 1);

        /* TESTING */
        (void) send_message(tx_msg, ecu);
    }
    else if(
            (msg->header.type == HOBD_MSG_TYPE_QUERY)
            &&
            ((msg->header.subtype == HOBD_MSG_SUBTYPE_TABLE_SUBGROUP)
            || (msg->header.subtype == HOBD_MSG_SUBTYPE_TABLE)))
    {
        /* table/subtable query */
        /* TODO - is the format the same for both types? */
        const hobd_data_table_query_s * const query =
                (hobd_data_table_query_s*) &msg->data[0];

        hobd_data_table_response_s * const resp =
                (hobd_data_table_response_s*) &tx_msg->data[0];

        /* TODO - support full table query */
        assert(msg->header.subtype == HOBD_MSG_SUBTYPE_TABLE_SUBGROUP);
        assert(((int) query->offset + (int) query->count) <= HOBD_TABLE_SIZE_MAX);

        /* TODO - check table max size - probably should be < 255 */

        /* fill response */
        tx_msg->header.type = HOBD_MSG_TYPE_RESPONSE;
        tx_msg->header.size = HOBD_MSG_HEADERCS_SIZE;
        tx_msg->header.subtype = msg->header.subtype;

        tx_msg->header.size += (uint8_t) sizeof(*resp);
        tx_msg->header.size += query->count;

        resp->table = query->table;
        resp->offset = query->offset;

        if(query->count != 0)
        {
            uint8_t * const data_table = get_data_table(query->table, ecu);
            assert(data_table != NULL);

            (void) memcpy(
                    &resp->data[0],
                    &data_table[query->offset],
                    query->count);
        }

        tx_msg->data[tx_msg->header.size - HOBD_MSG_HEADER_SIZE - 1] = hobd_parser_checksum(
                (uint8_t*) tx_msg,
                tx_msg->header.size - 1);

        /* TESTING */
        (void) send_message(tx_msg, ecu);
    }
}

static void reset_state(
        ecu_s * const ecu)
{
    ecu->state = STATE_WAIT4_WAKEUP;

    hobd_parser_reset(&ecu->rx_parser);

    printf("->STATE_WAIT4_WAKEUP\n");
}

static void wait4_wakeup(
        ecu_s * const ecu)
{
    const uint8_t msg_type = check_for_message(ecu);

    const hobd_msg_header_s * const header =
            (hobd_msg_header_s*) ecu->rx_buffer;

    if(msg_type == HOBD_MSG_TYPE_WAKE_UP)
    {
        if(header->subtype == HOBD_MSG_SUBTYPE_WAKE_UP)
        {
            ecu->state = STATE_WAIT4_DIAG_INIT;
            hobd_parser_reset(&ecu->rx_parser);
            printf("->STATE_WAIT4_DIAG_INIT\n");
        }
    }
}

static void wait4_diag_init(
        ecu_s * const ecu)
{
    const uint8_t msg_type = check_for_message(ecu);

    const hobd_msg_s * const msg =
            (hobd_msg_s*) ecu->rx_buffer;

    if(msg_type == HOBD_MSG_TYPE_QUERY)
    {
        if(msg->header.subtype == HOBD_MSG_SUBTYPE_INIT)
        {
            if(msg->data[0] == HOBD_INIT_DATA)
            {
                ecu->state = STATE_ACTIVE_LISTEN;
                hobd_parser_reset(&ecu->rx_parser);
                printf("->STATE_ACTIVE_LISTEN\n");

                handle_rx_message(msg, ecu);
            }
        }
    }
}

static void active_listen(
        ecu_s * const ecu)
{
    const uint8_t msg_type = check_for_message(ecu);

    const hobd_msg_s * const msg =
            (hobd_msg_s*) ecu->rx_buffer;

    /* only responding to querries */
    if(msg_type == HOBD_MSG_TYPE_QUERY)
    {
        handle_rx_message(msg, ecu);
    }
}

ecu_s *ecu_new(
        const int socket_fd)
{
    ecu_s * const ecu = calloc(1, sizeof(*ecu));

    ecu->socket_fd = socket_fd;

    hobd_parser_init(
            &ecu->rx_buffer[0],
            (uint16_t) sizeof(ecu->rx_buffer),
            &ecu->rx_parser);

    (void) memset(
            &ecu->table_10_page[0],
            0xAA,
            sizeof(ecu->table_10_page));

    (void) memset(
            &ecu->table_D1_page[0],
            0xBB,
            sizeof(ecu->table_D1_page));

    reset_state(ecu);

    return ecu;
}

int ecu_update(
        ecu_s * const ecu)
{
    int err = 0;

    if(ecu->state == STATE_WAIT4_WAKEUP)
    {
        wait4_wakeup(ecu);
    }
    else if(ecu->state == STATE_WAIT4_DIAG_INIT)
    {
        wait4_diag_init(ecu);
    }
    else if(ecu->state == STATE_ACTIVE_LISTEN)
    {
        active_listen(ecu);
    }
    else
    {
        assert(ecu->state != ecu->state);
    }

    return err;
}
