/**
 * @file root_task.h
 * @brief TODO.
 *
 */

#ifndef ROOT_TASK_H
#define ROOT_TASK_H

#include <stdint.h>

#include "init_env.h"

void root_task_init(
        const uint32_t virt_pool_size,
        const uint32_t mem_pool_size,
        char * const mem_pool,
        init_env_s * const env);

#endif /* ROOT_TASK_H */
