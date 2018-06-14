/**
 * @file main.c
 * @brief TODO.
 *
 */

#include <autoconf.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <sel4/sel4.h>
#include <vka/vka.h>
#include <vka/object.h>
#include <vka/object_capops.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>
#include <simple/simple.h>
#include <simple-default/simple-default.h>
#include <sel4platsupport/platsupport.h>
#include <sel4platsupport/io.h>
#include <sel4utils/vspace.h>
#include <sel4utils/page_dma.h>
#include <sel4utils/thread.h>
#include <sel4debug/debug.h>
#include <platsupport/io.h>
#include <utils/io.h>
#include <utils/zf_log.h>
#include <sel4utils/sel4_zf_logif.h>

#include "root_task.h"
#include "thread.h"

/* 32 * 4K = 128K */
#define MEM_POOL_SIZE ((1 << seL4_PageBits) * 32)

/* arbitrary (but unique) number for a badge */
#define EP_BADGE (0x61)

static root_task_s g_root_task;
static char g_mem_pool[MEM_POOL_SIZE];

static thread_s g_thread;

static void example_thread(
        void *arg0,
        void *arg1,
        void *ipc_buffer_vaddr)
{
    printf("\nhello from thread\n");
    printf(
            "arg0 = %p, arg1 = %p, ipc_buffer = %p\n\n",
            arg0,
            arg1,
            ipc_buffer_vaddr);
}

int main(
        int argc,
        char **argv)
{
    memset(&g_root_task, 0, sizeof(g_root_task));
    memset(&g_thread, 0, sizeof(g_thread));

    /* create the root task */
    root_task_init(
            MEM_POOL_SIZE,
            &g_mem_pool[0],
            &g_root_task);

    ZF_LOGD("root-task is initialized");

    /* create a new thread */
    thread_create(
            "example-thread",
            EP_BADGE,
            &g_root_task,
            &g_thread);

    /* start the new thread */
    thread_start(
            &example_thread,
            NULL,
            NULL,
            &g_thread);

    /* loop forever, servicing events/faults/etc */
    while(1)
    {
        /* forfeit the remainder of our timeslice */
        seL4_Yield();
    }

    /* should not get here, intentional halt */
    seL4_DebugHalt();

    return 0;
}
