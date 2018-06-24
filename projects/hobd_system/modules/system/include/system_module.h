/**
 * @file system_module.h
 * @brief TODO.
 *
 */

#ifndef SYSTEM_MODULE_H
#define SYSTEM_MODULE_H

#include <stdint.h>

#include "init_env.h"

/* called by the root task */
void system_module_init(
        init_env_s * const env);

/* MUST only be called from another thread */
void system_module_wait_for_start(void);

#endif /* SYSTEM_MODULE_H */
