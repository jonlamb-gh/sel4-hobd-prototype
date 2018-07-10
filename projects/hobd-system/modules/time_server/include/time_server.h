/**
 * @file time_server.h
 * @brief TODO.
 *
 */

#ifndef TIME_SERVER_H
#define TIME_SERVER_H

#include <stdint.h>

#include <utils/time.h>

/* TODO - not thread safe yet */

#define MS_TO_NS(ms) (ms * NS_IN_MS)
#define S_TO_NS(s) (MS_TO_NS(s * MS_IN_S))

/* function to call when a timeout comes in */
typedef int (*time_server_timeout_cb_fn_t)(uintptr_t token);

/* nanoseconds */
void time_server_get_time(
        uint64_t * const time);

void time_server_alloc_id(
        uint32_t * const id);

/* nanoseconds */
void time_server_register_periodic_cb(
        const uint64_t period,
        const uint64_t start,
        const uint32_t id,
        const time_server_timeout_cb_fn_t callback,
        uintptr_t token);

void time_server_register_rel_cb(
        const uint64_t time,
        const uint32_t id,
        const time_server_timeout_cb_fn_t callback,
        uintptr_t token);

void time_server_deregister_cb(
        const uint32_t id);

#endif /* TIME_SERVER_H */
