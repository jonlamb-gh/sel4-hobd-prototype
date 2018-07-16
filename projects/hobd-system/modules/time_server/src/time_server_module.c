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
#include <sync/mutex.h>
#include <allocman/vka.h>
#include <platsupport/delay.h>
#include <platsupport/ltimer.h>
#include <platsupport/time_manager.h>
#include <platsupport/local_time_manager.h>
#include <sel4utils/sel4_zf_logif.h>

#include "config.h"
#include "ipc_util.h"
#include "init_env.h"
#include "thread.h"
#include "time_server.h"
#include "time_server_module.h"

#ifdef TMSERVERMOD_DEBUG
#define MODLOGD(...) ZF_LOGD(__VA_ARGS__)
#else
#define MODLOGD(...)
#endif

#define MAX_TIMEOUTS (2)

#define ENDPOINT_BADGE IPC_ENDPOINT_BADGE(TMSERVERMOD_BASE_BADGE)

/* TODO - which things should be in the init_env? */

/* main timer notification that receives ltimer IRQ on */
static vka_object_t g_timer_ntfn;
static seL4_timer_t g_timer;
static time_manager_t g_tm;
static sync_mutex_t g_tm_mutex;

static thread_s g_thread;
static uint64_t g_thread_stack[TMSERVERMOD_STACK_SIZE];

static void tm_lock(void)
{
    const int err = sync_mutex_lock(&g_tm_mutex);
    ZF_LOGF_IF(err != 0, "Failed to lock tm mutex");
}

static void tm_unlock(void)
{
    const int err = sync_mutex_unlock(&g_tm_mutex);
    ZF_LOGF_IF(err != 0, "Failed to unlock tm mutex");
}

static void time_server_thread_fn(
        const seL4_CPtr ep_cap)
{
    int err;

    MODLOGD(TMSERVERMOD_THREAD_NAME " thread is running");

    while(1)
    {
        /* wait on the IRQ notification */
        seL4_Word mbadge = 0;
        seL4_Wait(g_timer_ntfn.cptr, &mbadge);

        sel4platsupport_handle_timer_irq(&g_timer, mbadge);

        tm_lock();

        err = tm_update(&g_tm);
        ZF_LOGF_IF(err != 0, "Failed to update time manager");

        tm_unlock();
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

    err = ltimer_reset(&g_timer.ltimer);
    ZF_LOGF_IF(err != 0, "Failed to reset timer");
}

static void init_tm(
        init_env_s * const env)
{
    int err;
    uint64_t init_time;

    err = tm_init(
            &g_tm,
            &g_timer.ltimer,
            &env->io_ops,
            MAX_TIMEOUTS);
    ZF_LOGF_IF(err != 0, "Failed to initialize local time manager");

    err = tm_get_time(&g_tm, &init_time);
    ZF_LOGF_IF(err != 0, "Failed to get time");

    MODLOGD("Created timer - current time is %llu ns", init_time);
}

void time_server_module_init(
        init_env_s * const env)
{
    int err;

    init_timer(env);

    /* create time manager mutex */
    err = sync_mutex_new(&env->vka, &g_tm_mutex);
    ZF_LOGF_IF(err != 0, "Failed to create new mutex");

    tm_lock();

    /* create a worker thread */
    thread_create(
            TMSERVERMOD_THREAD_NAME,
            ENDPOINT_BADGE,
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
    init_tm(env);

    /* set thread priority and affinity */
    thread_set_priority(TMSERVERMOD_THREAD_PRIORITY, &g_thread);
    thread_set_affinity(TMSERVERMOD_THREAD_AFFINITY, &g_thread);

    MODLOGD("%s is initialized", TMSERVERMOD_THREAD_NAME);

    /* start the new thread */
    thread_start(&g_thread);

    tm_unlock();
}

void time_server_get_time(
        uint64_t * const time)
{
    tm_lock();

    const int err = tm_get_time(&g_tm, time);
    ZF_LOGF_IF(err != 0, "Failed to get time");

    tm_unlock();
}

void time_server_alloc_id(
        uint32_t * const id)
{
    unsigned int tm_id = 0;

    tm_lock();

    const int err = tm_alloc_id(&g_tm, &tm_id);
    ZF_LOGF_IF(err != 0, "Failed to allocate time manager ID");

    tm_unlock();

    if(err == 0)
    {
        *id = (uint32_t) tm_id;
    }
}

void time_server_register_periodic_cb(
        const uint64_t period,
        const uint64_t start,
        const uint32_t id,
        const time_server_timeout_cb_fn_t callback,
        uintptr_t token)
{
    tm_lock();

    const int err = tm_register_periodic_cb(
            &g_tm,
            period,
            start,
            id,
            (timeout_cb_fn_t) callback,
            token);
    ZF_LOGF_IF(
            err != 0,
            "Failed to register periodic timeout callback with ID = 0x%lX",
            (unsigned long) id);

    tm_unlock();
}

void time_server_register_rel_cb(
        const uint64_t time,
        const uint32_t id,
        const time_server_timeout_cb_fn_t callback,
        uintptr_t token)
{
    tm_lock();

    const int err = tm_register_rel_cb(
            &g_tm,
            time,
            id,
            (timeout_cb_fn_t) callback,
            token);
    ZF_LOGF_IF(
            err != 0,
            "Failed to register relative timeout callback with ID = 0x%lX",
            (unsigned long) id);

    tm_unlock();
}

void time_server_deregister_cb(
        const uint32_t id)
{
    tm_lock();

    const int err = tm_deregister_cb(&g_tm, (unsigned int) id);
    ZF_LOGF_IF(
            err != 0,
            "Failed to deregister timeout callback with ID = 0x%lX",
            (unsigned long) id);

    tm_unlock();
}
