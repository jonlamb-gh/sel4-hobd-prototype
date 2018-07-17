/**
 * @file main.c
 * @brief TODO.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <popt.h>

#include "mmc_entry.h"
#include "hobd_kline.h"

static const char DEFAULT_FILE_NAME[] = "hobd.log";

static const char DEFAULT_T10_CSV_FILE_NAME[] = "table_10.csv";
static const char DEFAULT_TD1_CSV_FILE_NAME[] = "table_D1.csv";

static const char * const ENTRY_TYPE_STR_TABLE[] =
{
    "INVALID",
    "NA",
    "BEGIN-FLAG",
    "HOBD-MSG",
    "FAULT",
    "HEARTBEAT",
    NULL
};

/* global variables */
static volatile sig_atomic_t g_exit_signaled;
static uint8_t g_entry_buffer[2 * MMC_ENTRY_SIZE_MAX];

static void sig_handler(
        int sig)
{
    if(sig == SIGINT)
    {
        g_exit_signaled = 1;
    }
}

static const mmc_entry_s *read_next_entry(
        FILE * const file)
{
    mmc_entry_s *msg =
            (mmc_entry_s*) &g_entry_buffer[0];

    /* read header */
    const size_t bytes_read = fread(
            (void*) &msg->header,
            1,
            sizeof(msg->header),
            file);

    /* check status if not at EOF */
    if(feof(file) == 0)
    {
        assert(bytes_read == MMC_ENTRY_HEADER_SIZE);
    }
    else
    {
        /* at EOF */
        msg = NULL;
    }

    /* read data payload */
    if((msg != NULL) && (msg->header.size != 0))
    {
        const size_t data_bytes_read = fread(
                (void*) &msg->data[0],
                1,
                (size_t) msg->header.size,
                file);
        assert(data_bytes_read == (size_t) msg->header.size);
    }

    return (const mmc_entry_s*) msg;
}

static const char *get_entry_type_str(
        const uint16_t type)
{
    if(type == MMC_ENTRY_TYPE_INVALID)
    {
        return ENTRY_TYPE_STR_TABLE[0];
    }
    else if(type == MMC_ENTRY_TYPE_BEGIN_FLAG)
    {
        return ENTRY_TYPE_STR_TABLE[2];
    }
    else if(type == MMC_ENTRY_TYPE_HOBD_MSG)
    {
        return ENTRY_TYPE_STR_TABLE[3];
    }
    else if(type == MMC_ENTRY_TYPE_FAULT)
    {
        return ENTRY_TYPE_STR_TABLE[4];
    }
    else if(type == MMC_ENTRY_TYPE_HEARTBEAT)
    {
        return ENTRY_TYPE_STR_TABLE[5];
    }
    else
    {
        return ENTRY_TYPE_STR_TABLE[1];
    }
}

static void print_hobd_msg(
        const hobd_msg_s * const msg)
{
    printf("  hobd-msg\n");

    printf("    type: 0x%02lX\n", (unsigned long) msg->header.type);
    printf("    size: %lu bytes\n", (unsigned long) msg->header.size);
    printf("    subtype: 0x%02lX\n", (unsigned long) msg->header.subtype);
    printf(
            "    checksum: 0x%02lX\n",
            (unsigned long) msg->data[msg->header.size - HOBD_MSG_HEADERCS_SIZE]);

    if(msg->header.type == HOBD_MSG_TYPE_QUERY)
    {
        printf("      -> query\n");

        if(msg->header.subtype == HOBD_MSG_SUBTYPE_TABLE_SUBGROUP)
        {
            const hobd_data_table_query_s * query =
                    (const hobd_data_table_query_s*) &msg->data[0];

            printf("        table-subgroup\n");
            printf("        table: 0x%02lX\n", (unsigned long) query->table);
            printf("        offset: 0x%02lX\n", (unsigned long) query->offset);
            printf("        count: %lu\n", (unsigned long) query->count);
        }
    }
    else if(msg->header.type == HOBD_MSG_TYPE_RESPONSE)
    {
        printf("      <- response\n");

        if(msg->header.subtype == HOBD_MSG_SUBTYPE_TABLE_SUBGROUP)
        {
            const hobd_data_table_response_s * resp =
                    (const hobd_data_table_response_s*) &msg->data[0];

            printf("        table-subgroup\n");
            printf("        table: 0x%02lX\n", (unsigned long) resp->table);
            printf("        offset: 0x%02lX\n", (unsigned long) resp->offset);
            printf(
                    "        count: %lu\n",
                    (unsigned long) (msg->header.size - HOBD_MSG_HEADERCS_SIZE - sizeof(*resp)));
        }
        else if(msg->header.subtype == HOBD_MSG_SUBTYPE_INIT)
        {
            printf("        init\n");
        }
    }
    else if(msg->header.type == HOBD_MSG_TYPE_WAKE_UP)
    {
        printf("      -> wake-up\n");
    }
}

static void print_entry(
        const uint32_t entry_count,
        const mmc_entry_s * const entry)
{
    assert(entry != NULL);

    printf("[%04lu]\n", (unsigned long) entry_count);

    printf(
            "  type: 0x%04lX | %s\n  size: %lu bytes\n  timestamp: %llu ns\n",
            (unsigned long) entry->header.type,
            get_entry_type_str(entry->header.type),
            (unsigned long) entry->header.size,
            (unsigned long long) entry->header.timestamp);

    if(entry->header.type == MMC_ENTRY_TYPE_HOBD_MSG)
    {
        print_hobd_msg((const hobd_msg_s*) &entry->data[0]);
    }

    printf("\n");
    (void) fflush(stdout);
}

static void log_csv_headers(
        FILE * const t10_file,
        FILE * const td1_file)
{
    assert(t10_file != NULL);
    assert(td1_file != NULL);

    fprintf(
            t10_file,
            "timestamp engine_rpm tps_volt tps_percent ect_volt ect_temp "
            "iat_volt iat_temp map_volt map_pressure reserved_0 reserved_1 "
            "battery_volt wheel_speed fuel_injectors\n");

    fprintf(
            td1_file,
            "timestamp gear reserved_0 reserved_1 reserved_2 engine_on\n");
}

static void log_csv_table_10(
        const unsigned long long timestamp,
        const hobd_table_10_s * const table,
        FILE * const file)
{
    fprintf(
            file,
            "%llu %u %u %u %u %u"
            "%u %u %u %u %u %u"
            "%u %u %u\n",
            timestamp,
            (unsigned int) table->engine_rpm,
            (unsigned int) table->tps_volt,
            (unsigned int) table->tps_percent,
            (unsigned int) table->ect_volt,
            (unsigned int) table->ect_temp,
            (unsigned int) table->iat_volt,
            (unsigned int) table->iat_temp,
            (unsigned int) table->map_volt,
            (unsigned int) table->map_pressure,
            (unsigned int) table->reserved_0,
            (unsigned int) table->reserved_1,
            (unsigned int) table->battery_volt,
            (unsigned int) table->wheel_speed,
            (unsigned int) table->fuel_injectors);
}

static void log_csv_table_d1(
        const unsigned long long timestamp,
        const hobd_table_d1_s * const table,
        FILE * const file)
{
    fprintf(
            file,
            "%llu %u %u %u %u %u\n",
            timestamp,
            (unsigned int) table->gear,
            (unsigned int) table->reserved_0,
            (unsigned int) table->reserved_1,
            (unsigned int) table->reserved_2,
            (unsigned int) table->engine_on);
}

static void log_csv_entry(
        const mmc_entry_s * const entry,
        FILE * const t10_file,
        FILE * const td1_file)
{
    assert(entry != NULL);
    assert(t10_file != NULL);
    assert(td1_file != NULL);

    if(entry->header.type == MMC_ENTRY_TYPE_HOBD_MSG)
    {
        const hobd_msg_s * const hobd_msg =
                (const hobd_msg_s*) &entry->data[0];

        if(hobd_msg->header.type == HOBD_MSG_TYPE_RESPONSE)
        {
            /* TODO - support both types */
            if(hobd_msg->header.subtype == HOBD_MSG_SUBTYPE_TABLE_SUBGROUP)
            {
                const hobd_data_table_response_s * const resp =
                        (const hobd_data_table_response_s*) &hobd_msg->data[0];

                if(resp->table == HOBD_TABLE_10)
                {
                    log_csv_table_10(
                            (unsigned long long) entry->header.timestamp,
                            (const hobd_table_10_s*) &resp->data[0],
                            t10_file);
                }
                else if(resp->table == HOBD_TABLE_D1)
                {
                    log_csv_table_d1(
                            (unsigned long long) entry->header.timestamp,
                            (const hobd_table_d1_s*) &resp->data[0],
                            td1_file);
                }
            }
        }
    }
}

int main(
        int argc,
        char **argv)
{
    int err = 0;
    uint32_t entry_count = 0;
    struct sigaction sigact;

    (void) memset(&sigact, 0, sizeof(sigact));
    g_exit_signaled = 0;
    sigact.sa_flags = SA_RESTART;
    sigact.sa_handler = sig_handler;

    err = sigaction(SIGINT, &sigact, 0);
    assert(err == 0);

    FILE * const input_file = fopen(DEFAULT_FILE_NAME, "rb");
    assert(input_file != NULL);

    FILE * const t10_csv_file = fopen(DEFAULT_T10_CSV_FILE_NAME, "w+");
    assert(t10_csv_file != NULL);

    FILE * const td1_csv_file = fopen(DEFAULT_TD1_CSV_FILE_NAME, "w+");
    assert(td1_csv_file != NULL);

    log_csv_headers(t10_csv_file, td1_csv_file);

    while(g_exit_signaled == 0)
    {
        const mmc_entry_s * const entry = read_next_entry(input_file);

        if(entry != NULL)
        {
            print_entry(entry_count, entry);
            entry_count += 1;

            log_csv_entry(entry, t10_csv_file, td1_csv_file);
        }

        if((err != 0) || (entry == NULL))
        {
            g_exit_signaled = 1;
        }
    }

    if(input_file != NULL)
    {
        (void) fclose(input_file);
    }

    if(t10_csv_file != NULL)
    {
        (void) fclose(t10_csv_file);
    }

    if(td1_csv_file != NULL)
    {
        (void) fclose(td1_csv_file);
    }

    if(err == 0)
    {
        err = EXIT_SUCCESS;
    }
    else
    {
        err = EXIT_FAILURE;
    }

    return err;
}
