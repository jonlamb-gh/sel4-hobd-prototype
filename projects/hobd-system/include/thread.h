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
#include <vspace/vspace.h>

#include "init_env.h"

/* use a 4K frame for the IPC buffer */
#define THREAD_IPC_BUFFER_FRAME_SIZE_BITS (12)

typedef struct
{
    vka_object_t tcb_object;
    vka_object_t ipc_frame_object;
    seL4_IPCBuffer *ipc_buffer;
    vka_object_t ipc_ep;
    seL4_CPtr ipc_ep_cap;
} thread_s;

typedef void (*thread_run_function_type)(const seL4_CPtr ep_cap);

void thread_create(
        const char * const name,
        const seL4_Word ipc_badge,
        const uint32_t stack_size,
        uint64_t * const stack,
        const thread_run_function_type thread_fn,
        init_env_s * const env,
        thread_s * const thread);

void thread_set_priority(
        const int priority,
        thread_s * const thread);

void thread_set_affinity(
        const seL4_Word affinity,
        thread_s * const thread);

void thread_start(
        thread_s * const thread);

#endif /* THREAD_H */
