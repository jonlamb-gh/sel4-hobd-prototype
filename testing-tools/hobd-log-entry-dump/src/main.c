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

    FILE * const file = fopen(DEFAULT_FILE_NAME, "rb");
    assert(file != NULL);

    while(g_exit_signaled == 0)
    {
        const mmc_entry_s * const entry = read_next_entry(file);

        if(entry != NULL)
        {
            print_entry(entry_count, entry);
            entry_count += 1;
        }

        if((err != 0) || (entry == NULL))
        {
            g_exit_signaled = 1;
        }
    }

    if(file != NULL)
    {
        (void) fclose(file);
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
