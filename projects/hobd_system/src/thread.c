/**
 * @file thread.c
 * @brief TODO.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <sel4/sel4.h>
#include <vspace/vspace.h>
#include <vka/object.h>
#include <vka/object_capops.h>
#include <sel4utils/vspace.h>
#include <sel4utils/mapping.h>
#include <sel4utils/thread.h>
#include <utils/arith.h>
//#include <sel4utils/sel4_zf_logif.h>
#include <sel4debug/debug.h>

#include "root_task.h"
#include "thread.h"

void thread_create(
        const char * const name,
        const seL4_Word ipc_badge,
        root_task_s * const root_task,
        thread_s * const thread)
{
    /* create a new thread configuration */
    thread->config = thread_config_default(
            &root_task->simple,
            simple_get_cnode(&root_task->simple),
            ipc_badge,
            seL4_CapNull, /* TODO fault_endpoint */
            seL4_MaxPrio);

    /* create a new thread */
    const int err = sel4utils_configure_thread_config(
            &root_task->vka,
            &root_task->vspace,
            &root_task->vspace,
            thread->config,
            &thread->ctx);
    ZF_LOGF_IF(err != 0, "Failed to configure new thread\n");

    NAME_THREAD(thread->ctx.tcb.cptr, name);
}

void thread_start(
        const sel4utils_thread_entry_fn thread_fn,
        void * const arg0,
        void * const arg1,
        thread_s * const thread)
{
    const int err = sel4utils_start_thread(
            &thread->ctx,
            thread_fn,
            arg0,
            arg1,
            1);
    ZF_LOGF_IF(err != 0, "Failed to start new thread\n");
}
