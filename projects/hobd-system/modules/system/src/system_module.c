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
#include "ipc_util.h"
#include "init_env.h"
#include "thread.h"
#include "time_server.h"
#include "mmc_entry.h"
#include "mmc.h"
#include "system_module.h"

#ifdef SYSMOD_DEBUG
#define MODLOGD(...) ZF_LOGD(__VA_ARGS__)
#else
#define MODLOGD(...)
#endif

#define HEARTBEAT_DELAY_SEC (4)
#define HEARTBEAT_TIMEOUT_NS S_TO_NS(HEARTBEAT_DELAY_SEC)

#define ENDPOINT_BADGE IPC_ENDPOINT_BADGE(SYSMOD_BASE_BADGE)

#define IPC_MSG_TYPE_TIMEOUT IPC_MSG_TYPE_ID(SYSMOD_BASE_BADGE, 1)

static volatile seL4_Word g_is_init = 0;
static sync_mutex_t g_sys_ready_mutex;

static uint32_t g_timeout_id;

static thread_s g_thread;
static uint64_t g_thread_stack[SYSMOD_STACK_SIZE];

static void signal_ready_to_start(void)
{
    const int err = sync_mutex_unlock(&g_sys_ready_mutex);
    ZF_LOGF_IF(err != 0, "Failed to signal system ready to start - failed to unlock mutex");

    MODLOGD("System starting");
}

static int timeout_cb(
        uintptr_t token)
{
    const seL4_MessageInfo_t msg_info =
            seL4_MessageInfo_new(IPC_MSG_TYPE_TIMEOUT, 0, 0, 0);

    /* non-blocking so this won't block the time server */
    seL4_NBSend(g_thread.ipc_ep_cap, msg_info);

    return 0;
}

static void sys_thread_fn(
        const seL4_CPtr ep_cap)
{
    int err;
    seL4_Word badge;

    /* setup the system-ready mutex mechanism */
    if(g_is_init == 0)
    {
        err = sync_mutex_lock(&g_sys_ready_mutex);
        ZF_LOGF_IF(err != 0, "Failed to lock mutex");

        MODLOGD("System ready to start");

        g_is_init = 1;
    }

    time_server_alloc_id(&g_timeout_id);

    time_server_register_periodic_cb(
            HEARTBEAT_TIMEOUT_NS,
            0,
            g_timeout_id,
            &timeout_cb,
            0);

    /* start the system, enabling any blocked threads on our mutex */
    signal_ready_to_start();

    /* forfeit remainder of our time slice */
    seL4_Yield();

    while(1)
    {
        const seL4_MessageInfo_t info = seL4_Recv(
                ep_cap,
                &badge);

        ZF_LOGF_IF(badge != ENDPOINT_BADGE, "Invalid IPC badge");

        const seL4_Word msg_label = seL4_MessageInfo_get_label(info);

        if(msg_label == IPC_MSG_TYPE_TIMEOUT)
        {
            /* log a heartbeat entry, non-blocking true so it could be dropped */
            mmc_log_entry_data(
                    MMC_ENTRY_TYPE_HEARTBEAT,
                    0,
                    NULL,
                    NULL,
                    1);
        }
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
            ENDPOINT_BADGE,
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
