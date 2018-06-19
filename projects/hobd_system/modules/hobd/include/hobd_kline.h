/**
 * @file hobd_kline.h
 * @brief Honda OBD K-line Protocol.
 *
 * @todo hobd protocol notes, how it relates to ISO 9141-2, etc.
 *
 * Data rate: 10,400 bps
 *
 * Initialization sequence:
 *   - pull k-line low for 70 ms
 *   - return k-line to high state
 *   - wait 120 ms
 *   - send wake up message - no response is expected
 *     - send {0xFE, 0x04, 0xFF, 0xFF}
 *   - send initialization command message - expect a response
 *     - send {0x72, 0x05, 0x00, 0xF0, 0x99}
 *     - expect {0x02, 0x04, 0x00, 0xFA}
 *
 * Checksum:
 * @todo - checksum notes.
 *
 */

#ifndef HOBD_KLINE_H
#define HOBD_KLINE_H

#include <stdint.h>

#define HOBD_MSG_HEADER_SIZE (3)
#define HOBD_MSG_CHECKSUM_SIZE (1)
#define HOBD_MSG_HEADERCS_SIZE (4)
#define HOBD_MSG_DATA_SIZE_MAX (251)
#define HOBD_MSG_SIZE_MIN (4)
#define HOBD_MSG_SIZE_MAX (255)

#define HOBD_MSG_TYPE_INVALID (0x00)
#define HOBD_MSG_TYPE_QUERY (0x72)
#define HOBD_MSG_TYPE_RESPONSE (0x02)
#define HOBD_MSG_TYPE_WAKE_UP (0xFE)

#define HOBD_MSG_SUBTYPE_INVALID (0xAA)
#define HOBD_MSG_SUBTYPE_WAKE_UP (0xFF)
#define HOBD_MSG_SUBTYPE_INIT (0x00)
#define HOBD_MSG_SUBTYPE_TABLE (0x71)
#define HOBD_MSG_SUBTYPE_TABLE_SUBGROUP (0x72)

#define HOBD_INIT_DATA (0xF0)

#define HOBD_TABLE_SIZE_MIN (1)
#define HOBD_TABLE_SIZE_MAX (255)

/* numbers are hex (_10 == 0x10 == d16) */
#define HOBD_TABLE_0 (0x00)
#define HOBD_TABLE_10 (0x10)
#define HOBD_TABLE_20 (0x20)
#define HOBD_TABLE_D1 (0xD1)

#define HOBD_TABLE_D1_TRANSMISSION_STATE_GEAR (0)
#define HOBD_TABLE_D1_TRANSMISSION_STATE_NEUTRAL (1)
#define HOBD_TABLE_D1_TRANSMISSION_STATE_KICKSTAND (3)

#define HOBD_TABLE_D1_ENGINE_STATE_OFF (0)
#define HOBD_TABLE_D1_ENGINE_STATE_ON (1)

typedef struct
{
    uint8_t type;
    uint8_t size;
    uint8_t subtype;
} hobd_msg_header_s;

typedef struct
{
    hobd_msg_header_s header;
    uint8_t data[0];
    // checksum
} hobd_msg_s;

typedef struct
{
    uint8_t table;
    uint8_t offset;
    uint8_t count;
} hobd_data_table_query_s;

typedef struct
{
    uint8_t table;
    uint8_t offset;
    uint8_t data[0];
} hobd_data_table_response_s;

typedef struct
{
    uint8_t data;
} hobd_data_init_s;

typedef struct
{
    uint16_t engine_rpm;
    uint8_t tps_volt;
    uint8_t tps_percent;
    uint8_t ect_volt;
    uint8_t ect_temp;
    uint8_t iat_volt;
    uint8_t iat_temp;
    uint8_t map_volt;
    uint8_t map_pressure;
    uint8_t reserved_0;
    uint8_t reserved_1;
    uint8_t battery_volt;
    uint8_t wheel_speed;
    uint16_t fuel_injectors;
} hobd_table_10_s;

typedef struct
{
    uint8_t gear;
    uint8_t reserved_0;
    uint8_t reserved_1;
    uint8_t reserved_2;
    uint8_t engine_on;
} hobd_table_d1_s;

#endif /* HOBD_KLINE_H */
