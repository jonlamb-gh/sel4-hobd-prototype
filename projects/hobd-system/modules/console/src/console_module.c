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
#include <platsupport/io.h>
#include <platsupport/mux.h>
#include <platsupport/gpio.h>
#include <platsupport/chardev.h>
#include <platsupport/serial.h>
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

#define CONSOLE_BAUD (115200UL)
#define CONSOLE_STARTUP_DELAY_MS (1500UL)

static microrl_t g_microrl_state;

static ps_chardevice_t g_cdev;

static thread_s g_thread;
static uint64_t g_thread_stack[CONSOLEMOD_STACK_SIZE];

/* prototypes */
static void print_help(void);

/* microrl API */
static void console_print(
        const char *str)
{
    uint32_t idx = 0;

    while(str[idx] != 0)
    {
        ps_cdev_putchar(
                &g_cdev,
                str[idx]);
        idx += 1;
    }
}

/* microrl API */
static int console_exec(
        int argc,
        const char * const * argv)
{
    /* TODO */
    print_help();

    return 0;
}

/* microrl API */
static void console_sigint_handler(void)
{
    console_print("caught SIGINT\n");
}

/* TODO - hook up formatting */
static void print_help(void)
{
    console_print("help message here\n");
}

static void console_thread_fn(const seL4_CPtr ep_cap)
{
    MODLOGD(CONSOLEMOD_THREAD_NAME " thread is running");

    ps_mdelay(CONSOLE_STARTUP_DELAY_MS);

    /* reconfigure the serial port */
    const int err = serial_configure(
            &g_cdev,
            CONSOLE_BAUD,
            8,
            PARITY_NONE,
            1);
    ZF_LOGF_IF(err != 0, "Failed to configure serial port");

    microrl_init(&g_microrl_state, &console_print);

    microrl_set_execute_callback(&g_microrl_state, &console_exec);

    microrl_set_sigint_callback(&g_microrl_state, &console_sigint_handler);

    microrl_set_complete_callback(&g_microrl_state, NULL);

    while(1)
    {
        const int data = ps_cdev_getchar(&g_cdev);

        if(data >= 0)
        {
            microrl_insert_char(&g_microrl_state, data);
        }
    }

    /* should not get here, intentional halt */
    seL4_DebugHalt();
}

static void init_cdev(
        init_env_s * const env)
{
    /* initialize character device - PS_SERIAL1 = IMX_UART2 - sabre lite console */
    const ps_chardevice_t * const char_dev = ps_cdev_init(
            PS_SERIAL1,
            &env->io_ops,
            &g_cdev);
    ZF_LOGF_IF(char_dev == NULL, "Failed to initialize character device");
}

void console_module_init(
        init_env_s * const env)
{
    init_cdev(env);

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
