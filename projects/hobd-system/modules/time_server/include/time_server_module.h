/**
 * @file time_server_module.h
 * @brief TODO.
 *
 */

#ifndef TIME_SERVER_MODULE_H
#define TIME_SERVER_MODULE_H

#include <stdint.h>

#include "init_env.h"

/* could make a seperate time_server.h ? */
/* TODO - not thread safe yet */

void time_server_module_init(
        init_env_s * const env);

/* nanoseconds */
void time_server_get_time(
        uint64_t * const time);

#endif /* TIME_SERVER_MODULE_H */
