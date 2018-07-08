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
#include <platsupport/ltimer.h>
#include <sel4utils/sel4_zf_logif.h>

#include "config.h"
#include "init_env.h"
#include "thread.h"
#include "time_server_module.h"
#include "system_module.h"
#include "hobd_kline.h"
#include "hobd_parser.h"
#include "hobd_msg.h"
#include "comm.h"
#include "hobd_module.h"

/* TODO - projects/util_libs/libplatsupport/src/plat/imx6/mux.c
 * doesn't seem to support all the GPIOs */
/* SD3_DAT7 - UART1_TX_DATA - GPIO6_IO17 */
//#define UART_TX_PORT (GPIO_BANK6)
//#define UART_TX_PIN (17)
#define UART_TX_PORT (GPIO_BANK1)
#define UART_TX_PIN (1)

#define MSG_RX_BUFFER_SIZE (512)
#define MSG_TX_BUFFER_SIZE (HOBD_MSG_SIZE_MAX + 1)

#define HOBD_KLINE_BAUD (10400UL)

static comm_s g_comm;

static thread_s g_thread;
static uint64_t g_thread_stack[HOBDMOD_STACK_SIZE];

static hobd_parser_s g_msg_parser;
static uint8_t g_msg_rx_buffer[MSG_RX_BUFFER_SIZE];
static uint8_t g_msg_tx_buffer[MSG_TX_BUFFER_SIZE];

static void ecu_init_seq(void)
{
    ZF_LOGD("Performing ECU GPIO initialization sequence");

    /* perform the init sequence */
    comm_gpio_init_seq(&g_comm.gpio_uart_tx, &g_comm);

    /* reconfigure the serial port */
    const int err = serial_configure(
            &g_comm.char_dev,
            HOBD_KLINE_BAUD,
            8,
            PARITY_NONE,
            1);
    ZF_LOGF_IF(err != 0, "Failed to configure serial port");
}

static void send_ecu_diag_messages(void)
{
    ZF_LOGD("Sending ECU diagnostic messages");

    hobd_msg_s * const msg = (hobd_msg_s*) &g_msg_tx_buffer[0];

    /* create a wake up message */
    (void) hobd_msg_no_data(
            HOBD_MSG_TYPE_WAKE_UP,
            HOBD_MSG_SUBTYPE_WAKE_UP,
            &g_msg_tx_buffer[0]);

    comm_send_msg(msg, &g_comm);

    ps_mdelay(1);

    /* create a diagnostic init message */
    msg->data[0] = HOBD_INIT_DATA;
    (void) hobd_msg(
            HOBD_MSG_TYPE_QUERY,
            HOBD_MSG_SUBTYPE_INIT,
            1,
            &g_msg_tx_buffer[0]);

    comm_send_msg(msg, &g_comm);
}

/* TODO - timeout */
static void wait_for_resp(
        const uint8_t subtype)
{
    uint8_t resp_found = 0;

    while(resp_found == 0)
    {
        const hobd_msg_s * const msg = comm_recv_msg(
                &g_msg_parser,
                &g_comm);

        if(msg->header.type == HOBD_MSG_TYPE_RESPONSE)
        {
            if(msg->header.subtype == subtype)
            {
                resp_found = 1;
            }
        }
    }

    /* TODO - testing */
    uint64_t resp_time;
    time_server_get_time(&resp_time);
    ZF_LOGD("Response msg time is %llu ns", resp_time);
}

static void send_table_req(
        const uint8_t table)
{
    hobd_msg_s * const tx_msg = (hobd_msg_s*) &g_msg_tx_buffer[0];

    if(table == HOBD_TABLE_10)
    {
        comm_fill_msg_subgroub_10_query(tx_msg);
    }
    else if(table == HOBD_TABLE_D1)
    {
        comm_fill_msg_subgroub_d1_query(tx_msg);
    }
    else
    {
        assert(table != table);
    }

    comm_send_msg(tx_msg, &g_comm);
}

static void comm_update_state(void)
{
    /* TODO - error/non-blocking */
    if(g_comm.state == COMM_STATE_GPIO_INIT)
    {
        /* perform the ECU diagnostic init sequence */
        ecu_init_seq();
        g_comm.state = COMM_STATE_SEND_ECU_INIT;
    }
    else if(g_comm.state == COMM_STATE_SEND_ECU_INIT)
    {
        /* establish a diagnostics connection with the ECU */
        send_ecu_diag_messages();
        wait_for_resp(HOBD_MSG_SUBTYPE_INIT);
        g_comm.state = COMM_STATE_SEND_REQ0;
    }
    else if(g_comm.state == COMM_STATE_SEND_REQ0)
    {
        send_table_req(HOBD_TABLE_10);
        wait_for_resp(HOBD_MSG_SUBTYPE_TABLE_SUBGROUP);
        g_comm.state = COMM_STATE_SEND_REQ1;
    }
    else if(g_comm.state == COMM_STATE_SEND_REQ1)
    {
        send_table_req(HOBD_TABLE_D1);
        wait_for_resp(HOBD_MSG_SUBTYPE_TABLE_SUBGROUP);
        g_comm.state = COMM_STATE_SEND_REQ0;
    }
}

static void obd_comm_thread_fn(void)
{
    int err;

    /* wait for system ready */
    system_module_wait_for_start();

    /* initialize the HOBD parser */
    hobd_parser_init(
            &g_msg_rx_buffer[0],
            sizeof(g_msg_rx_buffer),
            &g_msg_parser);

    /* TODO - disable char dev ? */

    /* initialize UART TX GPIO */
    err = gpio_new(
            &g_comm.gpio_sys,
            GPIOID(UART_TX_PORT, UART_TX_PIN),
            GPIO_DIR_OUT,
            &g_comm.gpio_uart_tx);
    ZF_LOGF_IF(err != 0, "Failed to initialize GPIO port/pin");

    ZF_LOGD(HOBDMOD_THREAD_NAME " thread is running");

    g_comm.state = COMM_STATE_GPIO_INIT;

    /* TODO - handle bad data/comms/etc */
    while(1)
    {
        comm_update_state();
    }

    /* should not get here, intentional halt */
    seL4_DebugHalt();
}

static void init_gpio(
        init_env_s * const env)
{
    int err;

    /* initialize the MUX subsystem */
    err = mux_sys_init(
            &env->io_ops,
            NULL,
            &env->io_ops.mux_sys);
    ZF_LOGF_IF(err != 0, "Failed to initialize MUX subsystem");

    /* initialize the GPIO subsystem */
    err = gpio_sys_init(
            &env->io_ops,
            &g_comm.gpio_sys);
    ZF_LOGF_IF(err != 0, "Failed to initialize GPIO subsystem");
}

static void init_uart(
        init_env_s * const env)
{
    /* initialize character device - PS_SERIAL0 = IMX_UART1 */
    const ps_chardevice_t * const char_dev = ps_cdev_init(
            PS_SERIAL0,
            &env->io_ops,
            &g_comm.char_dev);
    ZF_LOGF_IF(char_dev == NULL, "Failed to initialize character device");
}

void hobd_module_init(
        init_env_s * const env)
{
    init_gpio(env);
    init_uart(env);

    /* create a worker thread */
    thread_create(
            HOBDMOD_THREAD_NAME,
            HOBDMOD_EP_BADGE,
            (uint32_t) sizeof(g_thread_stack),
            &g_thread_stack[0],
            &obd_comm_thread_fn,
            env,
            &g_thread);

    /* set thread priority and affinity */
    thread_set_priority(seL4_MaxPrio, &g_thread);
    thread_set_affinity(HOBDMOD_THREAD_AFFINITY, &g_thread);

    ZF_LOGD("%s is initialized", HOBDMOD_THREAD_NAME);

    /* start the new thread */
    thread_start(&g_thread);
}
