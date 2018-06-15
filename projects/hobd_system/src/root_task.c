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
#include <vka/object.h>
#include <vka/capops.h>
#include <simple/simple.h>
#include <simple-default/simple-default.h>
#include <sel4platsupport/platsupport.h>
#include <sel4utils/vspace.h>
#include <sel4utils/sel4_zf_logif.h>
#include <sel4debug/debug.h>
#include <platsupport/io.h>

#include "root_task.h"

#define ROOT_TASK_THREAD_NAME "hobd-init"

void root_task_init(
        const uint32_t virt_pool_size,
        const uint32_t mem_pool_size,
        char * const mem_pool,
        root_task_s * const root_task)
{
    int err;

    /* prefix the logger with task name */
    zf_log_set_tag_prefix(ROOT_TASK_THREAD_NAME);

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
    printf("------------------------------\n\n");
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
    err = sel4utils_bootstrap_vspace_with_bootinfo_leaky(
            &root_task->vspace,
            &root_task->alloc_data,
            // ./kernel/libsel4/include/sel4/bootinfo_types.h
            //seL4_CapInitThreadPD,
            simple_get_pd(&root_task->simple),
            &root_task->vka,
            root_task->boot_info);
    ZF_LOGF_IF(err != 0, "Failed to bootstrap vspace\n");

    /* fill the allocator with virtual memory */
    void *vaddr = NULL;
    root_task->virt_reservation = vspace_reserve_range(
            &root_task->vspace,
            virt_pool_size,
            seL4_AllRights,
            1,
            &vaddr);
    ZF_LOGF_IF(root_task->virt_reservation.res == NULL, "Failed to reserve a chunk of memory.\n");

    bootstrap_configure_virtual_pool(
            root_task->allocman,
            vaddr,
            virt_pool_size,
            simple_get_pd(&root_task->simple));

    /* Setup debug port: printf() is only reliable after this */
    platsupport_serial_setup_simple(
            &root_task->vspace,
            &root_task->simple,
            &root_task->vka);

    /* create a common fault endpoint */
    err = vka_alloc_endpoint(
            &root_task->vka,
            &root_task->global_fault_ep_obj);
    ZF_LOGF_IF(err != 0, "Failed to create global fault endpoint\n");

    root_task->global_fault_ep = root_task->global_fault_ep_obj.cptr;

    ZF_LOGD("Created global fault ep 0x%X", root_task->global_fault_ep);

    ZF_LOGD("Root task is initialized");
}
