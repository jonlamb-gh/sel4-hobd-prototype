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
#include <platsupport/delay.h>

#include "config.h"
#include "init_env.h"
#include "thread.h"
#include "system_module.h"
#include "hobd_kline.h"
#include "hobd_module.h"

/* TODO - projects/util_libs/libplatsupport/src/plat/imx6/mux.c
 * doesn't seem to support all the GPIOs */
/* CSI0_DAT10, UART1_TX_DATA, GPIO5_IO28*/
//#define UART_TX_PORT (GPIO_BANK5)
//#define UART_TX_PIN (28)
#define UART_TX_PORT (GPIO_BANK1)
#define UART_TX_PIN (1)

static ps_chardevice_t g_char_dev;
static gpio_sys_t g_gpio_sys;
static thread_s g_thread;
static uint64_t g_thread_stack[HOBDMOD_STACK_SIZE];

static void hobd_kline_reset_seq(
        gpio_t * const gpio)
{
    int err;

    /* pull k-line low for 70 ms */
    err = gpio_clr(gpio);
    ZF_LOGF_IF(err != 0, "Failed to clear k-line GPIO\n");

    ps_mdelay(70);

    /* return to high, wait 120 ms */
    err = gpio_set(gpio);
    ZF_LOGF_IF(err != 0, "Failed to set k-line GPIO\n");

    ps_mdelay(120);

    /* TODO - send msg */
}

static void thread_fn(void)
{
    int err;
    gpio_t uart_tx_gpio;

    /* wait for system ready */
    system_module_wait_for_start();

    /* TODO - disable char dev ? */

    /* initialize UART TX GPIO */
    err = gpio_new(
            &g_gpio_sys,
            GPIOID(UART_TX_PORT, UART_TX_PIN),
            GPIO_DIR_OUT,
            &uart_tx_gpio);
    ZF_LOGF_IF(err != 0, "Failed to initialize GPIO port/pin\n");

    ZF_LOGD(HOBDMOD_THREAD_NAME " thread is running");

    /* perform the init sequence */
    hobd_kline_reset_seq(&uart_tx_gpio);

    while(1)
    {
        const int data = ps_cdev_getchar(&g_char_dev);

        if(data >= 0)
        {
            ZF_LOGD("got data: 0x%02X", (unsigned int) data);
        }
    }

    /* should not get here, intentional halt */
    seL4_DebugHalt();
}

static void init_gpio(
        init_env_s * const env)
{
    int err;

    (void) memset(&g_gpio_sys, 0, sizeof(g_gpio_sys));

    /* initialize the MUX subsystem */
    err = mux_sys_init(
            &env->io_ops,
            NULL,
            &env->io_ops.mux_sys);
    ZF_LOGF_IF(err != 0, "Failed to initialize MUX subsystem\n");

    /* initialize the GPIO subsystem */
    err = gpio_sys_init(
            &env->io_ops,
            &g_gpio_sys);
    ZF_LOGF_IF(err != 0, "Failed to initialize GPIO subsystem\n");
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

    /* create a worker thread */
    thread_create(
            HOBDMOD_THREAD_NAME,
            HOBDMOD_EP_BADGE,
            (uint32_t) sizeof(g_thread_stack),
            &g_thread_stack[0],
            &thread_fn,
            env,
            &g_thread);

    /* set thread priority and affinity */
    thread_set_priority(seL4_MaxPrio, &g_thread);
    thread_set_affinity(HOBDMOD_THREAD_AFFINITY, &g_thread);

    ZF_LOGD("%s is initialized", HOBDMOD_THREAD_NAME);

    /* start the new thread */
    thread_start(&g_thread);
}
