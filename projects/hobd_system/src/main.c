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
#include <sel4utils/thread.h>
#include <sel4utils/page_dma.h>
#include <sel4debug/debug.h>
#include <platsupport/io.h>
#include <utils/io.h>
#include <utils/zf_log.h>
#include <sel4utils/sel4_zf_logif.h>

#include "root_task.h"
#include "thread.h"

/* 32 * 4K = 128K */
#define MEM_POOL_SIZE ((1 << seL4_PageBits) * 32)

/* dimensions of virtual memory for the allocator to use */
#define ALLOCATOR_VIRTUAL_POOL_SIZE (BIT(seL4_PageBits) * 100)

/* size of the thread's stack in words */
#define THREAD_STACK_SIZE (512)

/* arbitrary (but unique) number for a badge */
#define EP_BADGE (0x61)

static root_task_s g_root_task;
static char g_mem_pool[MEM_POOL_SIZE];

static uint64_t g_thread_stack[THREAD_STACK_SIZE];
static thread_s g_thread;

static void example_thread(void)
{
    printf("\nhello from thread\n");

    /* fault */
    *((char*)0xDEADBEEF) = 0;

    printf("thread resumed\n");
}

int main(
        int argc,
        char **argv)
{
    memset(&g_root_task, 0, sizeof(g_root_task));
    memset(&g_thread, 0, sizeof(g_thread));

    /* create the root task */
    root_task_init(
            ALLOCATOR_VIRTUAL_POOL_SIZE,
            MEM_POOL_SIZE,
            &g_mem_pool[0],
            &g_root_task);

    /* create a new thread */
    thread_create(
            "example-thread",
            EP_BADGE,
            (uint32_t) sizeof(g_thread_stack),
            &g_thread_stack[0],
            &example_thread,
            &g_root_task,
            &g_thread);

    /* set thread priority */
    thread_set_priority(seL4_MaxPrio, &g_thread);

    /* start the new thread */
    thread_start(&g_thread);

#ifdef CONFIG_DEBUG_BUILD
    ZF_LOGD("Dumping scheduler");
    seL4_DebugDumpScheduler();
#endif

    /* loop forever, servicing events/faults/etc */
    while(1)
    {
        seL4_Word badge;

        const seL4_MessageInfo_t info = seL4_Recv(
                g_root_task.global_fault_ep,
                &badge);

        ZF_LOGD("Received fault on ep 0x%X - badge 0x%X", g_root_task.global_fault_ep, badge);

        sel4utils_print_fault_message(info, "fault-handler");
    }

    /* should not get here, intentional halt */
    seL4_DebugHalt();

    return 0;
}
