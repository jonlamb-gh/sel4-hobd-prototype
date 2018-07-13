/**
 * @file system_module.c
 * @brief TODO.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <sel4/sel4.h>
#include <sync/mutex.h>
#include <sel4debug/debug.h>
#include <platsupport/delay.h>

#include "config.h"
#include "init_env.h"
#include "thread.h"
#include "mmc_entry.h"
#include "mmc.h"
#include "system_module.h"

#ifdef SYSMOD_DEBUG
#define MODLOGD(...) ZF_LOGD(__VA_ARGS__)
#else
#define MODLOGD(...)
#endif

#define HEARTBEAT_DELAY_SEC (1)

static thread_s g_thread;
static uint64_t g_thread_stack[SYSMOD_STACK_SIZE];
static sync_mutex_t g_sys_ready_mutex;
static volatile seL4_Word g_is_init = 0;

static void signal_ready_to_start(void)
{
    const int err = sync_mutex_unlock(&g_sys_ready_mutex);
    ZF_LOGF_IF(err != 0, "Failed to signal system ready to start - failed to unlock mutex");

    MODLOGD("System starting");
}

static void sys_thread_fn(
        const seL4_CPtr ep_cap)
{
    int err;

    /* setup the system-ready mutex mechanism */
    if(g_is_init == 0)
    {
        err = sync_mutex_lock(&g_sys_ready_mutex);
        ZF_LOGF_IF(err != 0, "Failed to lock mutex");

        MODLOGD("System ready to start");

        g_is_init = 1;
    }

    /* start the system, enabling any blocked threads on our mutex */
    signal_ready_to_start();

    while(1)
    {
        /* log a heartbeat entry, non-blocking true so it could be dropped */
        mmc_log_entry_data(
                MMC_ENTRY_TYPE_HEARTBEAT,
                0,
                NULL,
                NULL,
                1);

        /* TODO - replace rough delay with periodic timer and endpoint */
        ps_sdelay(HEARTBEAT_DELAY_SEC);
    }

    /* should not get here, intentional halt */
    seL4_DebugHalt();
}

void system_module_init(
        init_env_s * const env)
{
    int err;
    (void) memset(&g_thread, 0, sizeof(g_thread));

    /* create the system-ready mutex */
    err = sync_mutex_new(&env->vka, &g_sys_ready_mutex);
    ZF_LOGF_IF(err != 0, "Failed to create new mutex");

    /* create the system thread */
    thread_create(
            SYSMOD_THREAD_NAME,
            SYSMOD_EP_BADGE,
            (uint32_t) sizeof(g_thread_stack),
            &g_thread_stack[0],
            &sys_thread_fn,
            env,
            &g_thread);

    /* set thread priority and affinity */
    thread_set_priority(SYSMOD_THREAD_PRIORITY, &g_thread);
    thread_set_affinity(SYSMOD_THREAD_AFFINITY, &g_thread);

    MODLOGD("%s is initialized", SYSMOD_THREAD_NAME);

    /* start the new thread */
    thread_start(&g_thread);
}

void system_module_wait_for_start(void)
{
    int err;

    while(g_is_init == 0)
    {
        ps_udelay(1);
    }

    err = sync_mutex_lock(&g_sys_ready_mutex);
    ZF_LOGF_IF(err != 0, "Failed to lock mutex");

    err = sync_mutex_unlock(&g_sys_ready_mutex);
    ZF_LOGF_IF(err != 0, "Failed to unlock mutex");
}
