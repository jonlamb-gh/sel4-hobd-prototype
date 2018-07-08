/**
 * @file time_server_module.h
 * @brief TODO.
 *
 */

#ifndef TIME_SERVER_MODULE_H
#define TIME_SERVER_MODULE_H

#include <stdint.h>

#include "init_env.h"

/* TODO - not thread safe yet */

void time_server_module_init(
        init_env_s * const env);

/* could make a seperate time_server.h ? */

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
        const time_server_timeout_cb_fn_t const callback,
        uintptr_t token);

#endif /* TIME_SERVER_MODULE_H */
