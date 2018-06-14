/**
 * @file root_task.c
 * @brief TODO.
 *
 */

#include <autoconf.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <sel4/sel4.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>
#include <simple/simple.h>
#include <simple-default/simple-default.h>
#include <sel4platsupport/platsupport.h>
#include <sel4utils/vspace.h>
#include <sel4utils/sel4_zf_logif.h>
#include <sel4debug/debug.h>

#include "root_task.h"

#define ROOT_TASK_THREAD_NAME "hobd-system"

void root_task_init(
        const uint32_t mem_pool_size,
        char * const mem_pool,
        root_task_s * const root_task)
{
    /* get boot info */
    root_task->boot_info = platsupport_get_bootinfo();
    ZF_LOGF_IF(root_task->boot_info == NULL, "Failed to get bootinfo\n");

    /* name this thread */
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(seL4_CapInitThreadTCB, ROOT_TASK_THREAD_NAME);
#endif

    /* init simple */
    simple_default_init_bootinfo(&root_task->simple, root_task->boot_info);

    /* print out bootinfo and other info about simple */
#ifdef CONFIG_DEBUG_BUILD
    simple_print(&root_task->simple);
#endif

    /* create an allocator */
    root_task->allocman = bootstrap_use_current_simple(
            &root_task->simple,
            mem_pool_size,
            mem_pool);
    ZF_LOGF_IF(root_task->allocman == NULL, "Failed to initialize alloc manager\n");

    /* create a vka (interface for interacting with the underlying allocator) */
    allocman_make_vka(&root_task->vka, root_task->allocman);

    /* create a vspace object to manage our vspace */
    const int err = sel4utils_bootstrap_vspace_with_bootinfo_leaky(
            &root_task->vspace,
            &root_task->alloc_data,
            // ./kernel/libsel4/include/sel4/bootinfo_types.h
            //seL4_CapInitThreadPD,
            simple_get_pd(&root_task->simple),
            &root_task->vka,
            root_task->boot_info);
    ZF_LOGF_IF(err != 0, "Failed to bootstrap vspace\n");

    /* Setup debug port: printf() is only reliable after this */
    platsupport_serial_setup_simple(NULL, &root_task->simple, &root_task->vka);
}
