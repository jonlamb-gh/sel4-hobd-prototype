/**
 * @file hobd_msg.h
 * @brief TODO.
 *
 */

#ifndef HOBD_MSG_H
#define	HOBD_MSG_H

#include <stdint.h>

#include <sel4utils/sel4_zf_logif.h>

#include "hobd_kline.h"

/* TODO - assertions/checks */

static inline void hobd_msg_update_checksum(
        hobd_msg_s * const msg)
{
    msg->data[msg->header.size - HOBD_MSG_HEADERCS_SIZE] = hobd_parser_checksum(
            (uint8_t*) msg,
            msg->header.size - 1);
}

/* no data payload, auto checksum */
static inline uint8_t hobd_msg_no_data(
        const uint8_t type,
        const uint8_t subtype,
        uint8_t * const buffer,
        const uint16_t buffer_size)
{
    hobd_msg_s * const msg = (hobd_msg_s*) &buffer[0];

    msg->header.type = type;
    msg->header.size = HOBD_MSG_HEADERCS_SIZE;
    msg->header.subtype = subtype;

    hobd_msg_update_checksum(msg);

    return msg->header.size;
}

/* data payload, auto checksum */
static inline uint8_t hobd_msg(
        const uint8_t type,
        const uint8_t subtype,
        const uint8_t data_size,
        uint8_t * const buffer,
        const uint16_t buffer_size)
{
    hobd_msg_s * const msg = (hobd_msg_s*) &buffer[0];

    msg->header.type = type;
    msg->header.size = data_size + HOBD_MSG_HEADERCS_SIZE;
    msg->header.subtype = subtype;

    hobd_msg_update_checksum(msg);

    return msg->header.size;
}

#endif /* HOBD_MSG_H */
