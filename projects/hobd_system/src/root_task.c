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

#include "platform.h"
#include "root_task.h"

#define ROOT_TASK_THREAD_NAME "hobd-init"

void root_task_init(
        const uint32_t virt_pool_size,
        const uint32_t mem_pool_size,
        char * const mem_pool,
        init_env_s * const env)
{
    int err;

    /* prefix the logger with task name */
    zf_log_set_tag_prefix(ROOT_TASK_THREAD_NAME);

    /* get boot info */
    env->boot_info = platsupport_get_bootinfo();
    ZF_LOGF_IF(env->boot_info == NULL, "Failed to get bootinfo\n");

    /* name this thread */
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(seL4_CapInitThreadTCB, ROOT_TASK_THREAD_NAME);
#endif

    /* init simple */
    simple_default_init_bootinfo(&env->simple, env->boot_info);

    /* print out bootinfo and other info about simple */
#ifdef CONFIG_DEBUG_BUILD
    simple_print(&env->simple);
    printf("------------------------------\n\n");
#endif

    /* create an allocator */
    env->allocman = bootstrap_use_current_simple(
            &env->simple,
            mem_pool_size,
            mem_pool);
    ZF_LOGF_IF(env->allocman == NULL, "Failed to initialize alloc manager\n");

    /* create a vka (interface for interacting with the underlying allocator) */
    allocman_make_vka(&env->vka, env->allocman);

    /* create a vspace object to manage our vspace */
    err = sel4utils_bootstrap_vspace_with_bootinfo_leaky(
            &env->vspace,
            &env->alloc_data,
            // ./kernel/libsel4/include/sel4/bootinfo_types.h
            //seL4_CapInitThreadPD,
            simple_get_pd(&env->simple),
            &env->vka,
            env->boot_info);
    ZF_LOGF_IF(err != 0, "Failed to bootstrap vspace\n");

    /* fill the allocator with virtual memory */
    void *vaddr = NULL;
    env->virt_reservation = vspace_reserve_range(
            &env->vspace,
            virt_pool_size,
            seL4_AllRights,
            1,
            &vaddr);
    ZF_LOGF_IF(env->virt_reservation.res == NULL, "Failed to reserve a chunk of memory.\n");

    bootstrap_configure_virtual_pool(
            env->allocman,
            vaddr,
            virt_pool_size,
            simple_get_pd(&env->simple));

    /* initialize platform */
    platform_init(env);

    /* create a common fault endpoint */
    err = vka_alloc_endpoint(
            &env->vka,
            &env->global_fault_ep_obj);
    ZF_LOGF_IF(err != 0, "Failed to create global fault endpoint\n");

    env->global_fault_ep = env->global_fault_ep_obj.cptr;

    ZF_LOGD("Created global fault ep 0x%X", env->global_fault_ep);

    ZF_LOGD("Root task is initialized");
}
