/**
 * @file time_server_module.h
 * @brief TODO.
 *
 */

#ifndef TIME_SERVER_MODULE_H
#define TIME_SERVER_MODULE_H

#include "init_env.h"

// TODO - make a time_server module with thread to handle IRQs
// TIMER refs
// https://github.com/seL4/sel4test/blob/master/apps/sel4test-driver/src/main.c#L179
// https://github.com/SEL4PROJ/rumprun-sel4-demoapps/blob/master/roottask/src/main.c#L279

void time_server_module_init(
        init_env_s * const env);

#endif /* TIME_SERVER_MODULE_H */
