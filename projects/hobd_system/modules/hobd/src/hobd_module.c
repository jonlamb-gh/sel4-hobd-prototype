/**
 * @file hobd_module.c
 * @brief TODO.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <sel4/sel4.h>
#include <sel4debug/debug.h>

#include "init_env.h"
#include "thread.h"
#include "hobd_module.h"

/* TODO - move to a config.h ? */

#define THREAD_NAME "hobd-module"

/* arbitrary (but unique) number for a badge */
#define EP_BADGE (0x61)

/* size of the thread's stack in words */
#define THREAD_STACK_SIZE (512)

static uint64_t g_thread_stack[THREAD_STACK_SIZE];
static thread_s g_thread;

static void thread_fn(void)
{
    /* prefix the logger with task name */
    zf_log_set_tag_prefix(THREAD_NAME);

    ZF_LOGW("thread is running, about to intentionally fault");

    /* fault */
    *((char*)0xDEADBEEF) = 0;

    ZF_LOGI("thread resumed");
}

void hobd_module_init(
        init_env_s * const env)
{
    (void) memset(&g_thread, 0, sizeof(g_thread));

    /* create a new thread */
    thread_create(
            THREAD_NAME,
            EP_BADGE,
            (uint32_t) sizeof(g_thread_stack),
            &g_thread_stack[0],
            &thread_fn,
            env,
            &g_thread);

    /* set thread priority */
    thread_set_priority(seL4_MaxPrio, &g_thread);

    /* start the new thread */
    thread_start(&g_thread);
}
