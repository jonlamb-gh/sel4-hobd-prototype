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
#include <platsupport/io.h>
#include <platsupport/mux.h>
#include <platsupport/gpio.h>
#include <platsupport/chardev.h>
#include <platsupport/serial.h>

#include "init_env.h"
#include "thread.h"
#include "hobd_module.h"

/* TODO - move to a config.h ? */
#define THREAD_NAME "hobd-module"

/* arbitrary (but unique) number for a badge */
#define EP_BADGE (0x61)

/* size of the thread's stack in words */
#define THREAD_STACK_SIZE (512)

static ps_chardevice_t g_char_dev;
static gpio_sys_t g_gpio_sys;
static thread_s g_thread;
static uint64_t g_thread_stack[THREAD_STACK_SIZE];

static void thread_fn(void)
{
    /* prefix the logger with task name */
    zf_log_set_tag_prefix(THREAD_NAME);

    ZF_LOGW("thread is running, about to intentionally fault");

    /* fault */
    *((char*)0xDEADBEEF) = 0;

    ZF_LOGI("thread resumed");
}

static void init_gpio(
        init_env_s * const env)
{
    int err;

    (void) memset(&g_gpio_sys, 0, sizeof(g_gpio_sys));

    /* initialize the MUX subsytem */
    err = mux_sys_init(
            &env->io_ops,
            NULL,
            &env->io_ops.mux_sys);
    ZF_LOGF_IF(err != 0, "Failed to initialize MUX subsystem\n");

    /* initialize the GPIO subsytem */
    err = gpio_sys_init(
            &env->io_ops,
            &g_gpio_sys);
    ZF_LOGF_IF(err != 0, "Failed to initialize GPIO subsystem\n");

    /* TODO - this is just a random port/pin combo */
    gpio_t gpio;
    err = gpio_new(
            &g_gpio_sys,
            GPIOID(GPIO_BANK1, 1),
            GPIO_DIR_OUT,
            &gpio);
    ZF_LOGF_IF(err != 0, "Failed to initialize GPIO port/pin\n");
}

static void init_uart(
        init_env_s * const env)
{
    int err;

    (void) memset(&g_char_dev, 0, sizeof(g_char_dev));

    /* initialize character device - PS_SERIAL0 = IMX_UART1 */
    const ps_chardevice_t * const char_dev = ps_cdev_init(
            PS_SERIAL0,
            &env->io_ops,
            &g_char_dev);
    ZF_LOGF_IF(char_dev == NULL, "Failed to initialize character device\n");

    /* line configuration of serial port */
    err = serial_configure(
            &g_char_dev,
            115200,
            8,
            PARITY_NONE,
            1);
    ZF_LOGF_IF(err != 0, "Failed to configure serial port\n");
}

void hobd_module_init(
        init_env_s * const env)
{
    (void) memset(&g_thread, 0, sizeof(g_thread));

    init_gpio(env);
    init_uart(env);

    /* create a new thread */
    thread_create(
            THREAD_NAME,
            EP_BADGE,
            (uint32_t) sizeof(g_thread_stack),
            &g_thread_stack[0],
            &thread_fn,
            env,
            &g_thread);

    /* set thread priority and affinity */
    thread_set_priority(seL4_MaxPrio, &g_thread);
    thread_set_affinity(1, &g_thread);

    ZF_LOGD("%s is initialized", THREAD_NAME);

    /* start the new thread */
    thread_start(&g_thread);
}
