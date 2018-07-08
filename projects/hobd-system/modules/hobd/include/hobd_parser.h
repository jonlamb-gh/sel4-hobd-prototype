/**
 * @file hobd_parser.h
 * @brief TODO.
 *
 */

#ifndef HOBD_PARSER_H
#define	HOBD_PARSER_H

#include <stdint.h>

#include "hobd_kline.h"

typedef enum
{
    HOBD_ERROR_NONE = 0,
    HOBD_ERROR_NO_MSG,
    HOBD_ERROR_MSG_TYPE,
    HOBD_ERROR_MSG_SUBTYPE,
    HOBD_ERROR_CHECKSUM,
} hobd_error_kind;

typedef enum
{
    HOBD_PARSER_STATE_GET_TYPE = 0,
    HOBD_PARSER_STATE_GET_SIZE,
    HOBD_PARSER_STATE_GET_SUBTYPE,
    HOBD_PARSER_STATE_GET_DATA,
    HOBD_PARSER_STATE_GET_CHECKSUM
} hobd_parser_state_kind;

/* TODO - update this */
typedef struct
{
    hobd_parser_state_kind state;
    uint8_t bytes_read;
    uint8_t total_size;
    uint16_t checksum;
    uint16_t valid_count;
    uint16_t invalid_count;
    uint8_t *rx_buffer;
    uint16_t rx_buffer_size;
} hobd_parser_s;

void hobd_parser_init(
        uint8_t * const rx_buffer,
        const uint16_t rx_buffer_size,
        hobd_parser_s * const parser);

void hobd_parser_reset(
        hobd_parser_s * const parser);

uint8_t hobd_parser_parse_byte(
        const uint8_t byte,
        hobd_parser_s * const parser);

uint8_t hobd_parser_checksum(
        const uint8_t * const data,
        const uint16_t size);

#endif /* HOBD_PARSER_H */
