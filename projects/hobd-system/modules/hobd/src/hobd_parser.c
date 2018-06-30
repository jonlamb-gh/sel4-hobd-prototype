/**
 * @file hobd_parser.c
 * @brief TODO.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <autoconf.h>
#include <sel4/sel4.h>
#include <utils/builtin.h>
#include <utils/zf_log_if.h>

#include "config.h"
#include "hobd_kline.h"
#include "hobd_parser.h"

static uint8_t valid_type(
        const uint8_t type)
{
    uint8_t valid_type;

    if(type == HOBD_MSG_TYPE_QUERY)
    {
        valid_type = type;
    }
    else if(type == HOBD_MSG_TYPE_RESPONSE)
    {
        valid_type = type;
    }
    else if(type == HOBD_MSG_TYPE_WAKE_UP)
    {
        valid_type = type;
    }
    else
    {
        valid_type = HOBD_ERROR_MSG_TYPE;
    }

    return valid_type;
}

static uint8_t valid_subtype(
        const uint8_t subtype)
{
    uint8_t valid_subtype;

    if(subtype == HOBD_MSG_SUBTYPE_WAKE_UP)
    {
        valid_subtype = subtype;
    }
    else if(subtype == HOBD_MSG_SUBTYPE_INIT)
    {
        valid_subtype = subtype;
    }
    else if(subtype == HOBD_MSG_SUBTYPE_TABLE_SUBGROUP)
    {
        valid_subtype = subtype;
    }
    else if(subtype == HOBD_MSG_SUBTYPE_TABLE)
    {
        valid_subtype = subtype;
    }
    else
    {
        valid_subtype = HOBD_ERROR_MSG_SUBTYPE;
    }

    return valid_subtype;
}

static __attribute__((always_inline)) inline void invalid_reset(
        hobd_parser_s * const parser)
{
    parser->invalid_count += 1;
    parser->state = HOBD_PARSER_STATE_GET_TYPE;
}

static __attribute__((always_inline)) inline void ready_parser(
        hobd_parser_s * const parser)
{
    parser->bytes_read = 0;
    parser->total_size = 0;
    parser->checksum = 0;
}

static __attribute__((always_inline)) inline void update_byte(
        const uint8_t byte,
        hobd_parser_s * const parser)
{
    parser->rx_buffer[parser->bytes_read] = byte;
    parser->bytes_read += 1;
    parser->checksum += (uint16_t) byte;
}

void hobd_parser_init(
        uint8_t * const rx_buffer,
        const uint16_t rx_buffer_size,
        hobd_parser_s * const parser)
{
    ZF_LOGF_IF(rx_buffer == NULL, "User provided Rx buffer is invalid\n");
    ZF_LOGF_IF(rx_buffer_size <= HOBD_MSG_SIZE_MIN, "User provided Rx buffer is too small\n");
    ZF_LOGF_IF(parser == NULL, "User provided parser is invalid\n");

    parser->state = HOBD_PARSER_STATE_GET_TYPE;

    ready_parser(parser);

    parser->valid_count = 0;
    parser->invalid_count = 0;
    parser->rx_buffer = rx_buffer;
    parser->rx_buffer_size = rx_buffer_size;
}

void hobd_parser_reset(
        hobd_parser_s * const parser)
{
    ready_parser(parser);
    parser->state = HOBD_PARSER_STATE_GET_TYPE;
}

uint8_t hobd_parser_parse_byte(
        const uint8_t byte,
        hobd_parser_s * const parser)
{
    uint8_t ret = HOBD_ERROR_NO_MSG;

    if(parser->state == HOBD_PARSER_STATE_GET_TYPE)
    {
        ready_parser(parser);
        update_byte(byte, parser);

        if(valid_type(byte) != HOBD_MSG_TYPE_INVALID)
        {
            parser->state = HOBD_PARSER_STATE_GET_SIZE;
        }
    }
    else if(parser->state == HOBD_PARSER_STATE_GET_SIZE)
    {
        update_byte(byte, parser);

        if(byte > HOBD_MSG_DATA_SIZE_MAX)
        {
            invalid_reset(parser);
        }
        else if(byte < HOBD_MSG_SIZE_MIN)
        {
            invalid_reset(parser);
        }
        else
        {
            parser->total_size = byte;
            parser->state = HOBD_PARSER_STATE_GET_SUBTYPE;
        }
    }
    else if(parser->state == HOBD_PARSER_STATE_GET_SUBTYPE)
    {
        update_byte(byte, parser);

        if(valid_subtype(byte) != HOBD_MSG_SUBTYPE_INVALID)
        {
            if(parser->total_size == HOBD_MSG_SIZE_MIN)
            {
                // no payload
                parser->state = HOBD_PARSER_STATE_GET_CHECKSUM;
            }
            else
            {
                parser->state = HOBD_PARSER_STATE_GET_DATA;
            }
        }
        else
        {
            invalid_reset(parser);
        }
    }
    else if(parser->state == HOBD_PARSER_STATE_GET_DATA)
    {
        update_byte(byte, parser);

        if((parser->bytes_read + 1) == parser->total_size)
        {
            parser->state = HOBD_PARSER_STATE_GET_CHECKSUM;
        }
    }
    else if(parser->state == HOBD_PARSER_STATE_GET_CHECKSUM)
    {
        parser->rx_buffer[parser->bytes_read] = byte;
        parser->bytes_read += 1;

        if(parser->checksum > 0x0100)
        {
            parser->checksum = (0x0100 - (parser->checksum & 0x00FF));
        }
        else
        {
            parser->checksum = (0x0100 - parser->checksum);
        }

        if(byte == (uint8_t) (parser->checksum & 0x00FF))
        {
            parser->valid_count += 1;
            ret = HOBD_ERROR_NONE;
        }
        else
        {
            parser->invalid_count += 1;
            ret = HOBD_ERROR_CHECKSUM;
        }

        parser->state = HOBD_PARSER_STATE_GET_TYPE;
    }

    return ret;
}

uint8_t hobd_parser_checksum(
        const uint8_t * const data,
        const uint16_t size)
{
    uint16_t cs;
    uint16_t idx;

    for(idx = 0, cs = 0; idx < size; idx += 1)
    {
        cs += (uint16_t) data[idx];
    }

    if(cs > 0x0100)
    {
        cs = (0x0100 - (cs & 0x00FF));
    }
    else
    {
        cs = (0x0100 - cs);
    }

    return (uint8_t) (cs & 0xFF);
}
