/**
 * @file thread.h
 * @brief TODO.
 *
 */

#ifndef THREAD_H
#define THREAD_H

#include <stdint.h>

#include <vka/object.h>
#include <vka/object_capops.h>
#include <sel4utils/thread.h>

#include "root_task.h"

typedef struct
{
    sel4utils_thread_config_t config;
    sel4utils_thread_t ctx;
} thread_s;

void thread_create(
        const char * const name,
        const seL4_Word ipc_badge,
        root_task_s * const root_task,
        thread_s * const thread);

void thread_start(
        const sel4utils_thread_entry_fn thread_fn,
        void * const arg0,
        void * const arg1,
        thread_s * const thread);

#endif /* THREAD_H */
