/**
 * @file time_server_module.c
 * @brief TODO.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <sel4/sel4.h>
#include <sel4debug/debug.h>
#include <sel4platsupport/timer.h>
#include <allocman/vka.h>
#include <platsupport/delay.h>
#include <platsupport/ltimer.h>
#include <platsupport/time_manager.h>
#include <platsupport/local_time_manager.h>
#include <sel4utils/sel4_zf_logif.h>

#include "config.h"
#include "init_env.h"
#include "thread.h"
#include "time_server_module.h"

/* TODO - which things should be in the init_env? */

/* main timer notification that receives ltimer IRQ on */
static vka_object_t g_timer_ntfn;
static seL4_timer_t g_timer;
static time_manager_t g_tm;

static thread_s g_thread;
static uint64_t g_thread_stack[TMSERVERMOD_STACK_SIZE];

static int timer_callback(uintptr_t id)
{
    ZF_LOGD("timer_callback");

    return 0;
}

static void time_server_thread_fn(void)
{
    int err;

    ZF_LOGD(TMSERVERMOD_THREAD_NAME " thread is running");

    while(1)
    {
        /* TODO */

        /* wait on the IRQ notification */
        seL4_Word mbadge = 0;
        seL4_Wait(g_timer_ntfn.cptr, &mbadge);

        ZF_LOGD("timer-server IRQ - badge = 0x%X", mbadge);

        sel4platsupport_handle_timer_irq(&g_timer, mbadge);

        err = tm_update(&g_tm);
        ZF_LOGF_IF(err != 0, "Failed to update time manager");
    }

    /* should not get here, intentional halt */
    seL4_DebugHalt();
}

static void init_timer(
        init_env_s * const env)
{
    int err;

    err = vka_alloc_notification(&env->vka, &g_timer_ntfn);
    ZF_LOGF_IF(err != 0, "Failed to create timer notification");

    err = sel4platsupport_init_default_timer_ops(
            &env->vka,
            &env->vspace,
            &env->simple,
            env->io_ops,
            g_timer_ntfn.cptr,
            &g_timer);
    ZF_LOGF_IF(err != 0, "Failed to initialize timer");
}

void time_server_module_init(
        init_env_s * const env)
{
    int err;

    init_timer(env);

    /* create a worker thread */
    thread_create(
            TMSERVERMOD_THREAD_NAME,
            TMSERVERMOD_EP_BADGE,
            (uint32_t) sizeof(g_thread_stack),
            &g_thread_stack[0],
            &time_server_thread_fn,
            env,
            &g_thread);

    /* bind timer notification to TCB */
    err = seL4_TCB_BindNotification(
            g_thread.tcb_object.cptr,
            g_timer_ntfn.cptr);
    ZF_LOGF_IF(err != 0, "Failed to bind timer notification to thread TCB");

    /* set up the timer manager */
    err = tm_init(
            &g_tm,
            &g_timer.ltimer,
            &env->io_ops,
            1);
    ZF_LOGF_IF(err != 0, "Failed to initialize local time manager");

    err = ltimer_reset(&g_timer.ltimer);
    ZF_LOGF_IF(err != 0, "Failed to reset timer");

    uint64_t time;
    err = ltimer_get_time(&g_timer.ltimer, &time);
    ZF_LOGF_IF(err != 0, "Failed to get time");

    ZF_LOGD("Created timer - current time is %llu ns", time);

    /* https://github.com/seL4/util_libs/blob/master/libplatsupport/src/mach/imx/ltimer.c#L121 */
    /* seems to be an issue there if current time is less than the amount to be subtracted ? */

    /* TESTING */
    unsigned int id;
    err = tm_alloc_id(&g_tm, &id);
    ZF_LOGF_IF(err != 0, "Failed to allocate time manager ID");

    ZF_LOGD("\nID = 0x%X", id);

    err = tm_register_periodic_cb(
            &g_tm,
            1000 * 500, // ns
            0, // start
            id,
            &timer_callback,
            id);
    ZF_LOGF_IF(err != 0, "Failed to register timer callback with ID = 0x%X", id);

    /* set thread priority and affinity */
    thread_set_priority(seL4_MaxPrio, &g_thread);
    thread_set_affinity(TMSERVERMOD_THREAD_AFFINITY, &g_thread);

    ZF_LOGD("%s is initialized", TMSERVERMOD_THREAD_NAME);

    /* start the new thread */
    thread_start(&g_thread);
}
