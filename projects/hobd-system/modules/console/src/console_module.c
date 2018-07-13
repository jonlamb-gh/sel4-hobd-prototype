/**
 * @file console_module.c
 * @brief TODO.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <sel4/sel4.h>
#include <sel4debug/debug.h>
#include <platsupport/delay.h>
//#include <sel4utils/sel4_zf_logif.h>

#include "microrl/microrl.h"

#include "config.h"
#include "init_env.h"
#include "thread.h"
#include "time_server.h"
#include "console_module.h"

#ifdef CONSOLEMOD_DEBUG
#define MODLOGD(...) ZF_LOGD(__VA_ARGS__)
#else
#define MODLOGD(...)
#endif

static thread_s g_thread;
static uint64_t g_thread_stack[CONSOLEMOD_STACK_SIZE];

static void console_thread_fn(const seL4_CPtr ep_cap)
{
    MODLOGD(CONSOLEMOD_THREAD_NAME " thread is running");

    while(1)
    {
        /* TODO */
    }

    /* should not get here, intentional halt */
    seL4_DebugHalt();
}

void console_module_init(
        init_env_s * const env)
{
    /* create a worker thread */
    thread_create(
            CONSOLEMOD_THREAD_NAME,
            CONSOLEMOD_EP_BADGE,
            (uint32_t) sizeof(g_thread_stack),
            &g_thread_stack[0],
            &console_thread_fn,
            env,
            &g_thread);

    /* set thread priority and affinity */
    thread_set_priority(seL4_MaxPrio, &g_thread);
    thread_set_affinity(CONSOLEMOD_THREAD_AFFINITY, &g_thread);

    MODLOGD("%s is initialized", CONSOLEMOD_THREAD_NAME);

    /* start the new thread */
    thread_start(&g_thread);
}
