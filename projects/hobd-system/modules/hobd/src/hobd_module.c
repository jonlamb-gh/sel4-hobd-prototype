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
#include "time_server.h"
#include "mmc_entry.h"
#include "mmc.h"
#include "system_module.h"
#include "hobd_kline.h"
#include "hobd_parser.h"
#include "hobd_msg.h"
#include "comm.h"
#include "hobd_module.h"

#ifdef HOBDMOD_DEBUG
#define MODLOGD(...) ZF_LOGD(__VA_ARGS__)
#else
#define MODLOGD(...)
#endif

/* see 36.4 IOMUXC of TRM */
/* SD3_DAT7 - UART1_TX_DATA - GPIO6_IO17 - SW_PAD_CTL_PAD_SD3_DATA7 */
#define UART_TX_PORT (GPIO_BANK6)
#define UART_TX_PIN (17)

#define MSG_RX_BUFFER_SIZE (512)
#define MSG_TX_BUFFER_SIZE (HOBD_MSG_SIZE_MAX + 1)

static comm_s g_comm;

static thread_s g_thread;
static uint64_t g_thread_stack[HOBDMOD_STACK_SIZE];

static hobd_parser_s g_msg_parser;
static uint8_t g_msg_rx_buffer[MSG_RX_BUFFER_SIZE];
static uint8_t g_msg_tx_buffer[MSG_TX_BUFFER_SIZE];

static void new_hobd_msg_callback(
        const hobd_msg_s * const msg,
        const uint64_t * const rx_timestamp)
{
    /* log the message as an MMC entry */
    mmc_log_entry_data(
            MMC_ENTRY_TYPE_HOBD_MSG,
            (uint16_t) msg->header.size,
            rx_timestamp,
            (const uint8_t*) msg,
            0);

    /* update the data tables with responses */
    /* TODO */
}

static void send_ecu_diag_messages(void)
{
    MODLOGD("Sending ECU diagnostic messages");

    hobd_msg_s * const msg = (hobd_msg_s*) &g_msg_tx_buffer[0];

    /* create a wake up message */
    (void) hobd_msg_no_data(
            HOBD_MSG_TYPE_WAKE_UP,
            HOBD_MSG_SUBTYPE_WAKE_UP,
            &g_msg_tx_buffer[0]);

    comm_send_msg(msg, &g_comm);

    /* TESTING - log tx messages */
    mmc_log_entry_data(
            MMC_ENTRY_TYPE_HOBD_MSG,
            HOBD_MSG_HEADERCS_SIZE,
            NULL,
            &g_msg_tx_buffer[0],
            0);

    ps_mdelay(1);

    /* create a diagnostic init message */
    msg->data[0] = HOBD_INIT_DATA;
    (void) hobd_msg(
            HOBD_MSG_TYPE_QUERY,
            HOBD_MSG_SUBTYPE_INIT,
            1,
            &g_msg_tx_buffer[0]);

    comm_send_msg(msg, &g_comm);

    /* TESTING - log tx messages */
    mmc_log_entry_data(
            MMC_ENTRY_TYPE_HOBD_MSG,
            HOBD_MSG_HEADERCS_SIZE + 1,
            NULL,
            &g_msg_tx_buffer[0],
            0);
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
        assert(false);
    }

    comm_send_msg(tx_msg, &g_comm);

    /* TESTING - log tx messages */
    mmc_log_entry_data(
            MMC_ENTRY_TYPE_HOBD_MSG,
            (uint16_t) tx_msg->header.size,
            NULL,
            (const uint8_t*) tx_msg,
            0);
}

static void comm_update_state(void)
{
    uint64_t rx_timestamp;

    /* TODO - error/non-blocking */
    /* TODO - better state managment, retry current state, fallback state, etc */

    if(g_comm.state == COMM_STATE_GPIO_INIT)
    {
        /* perform the ECU diagnostic init sequence */
        comm_ecu_init_seq(&g_comm);

        g_comm.state = COMM_STATE_SEND_ECU_INIT;
        MODLOGD("->STATE_SEND_ECU_INIT");
    }
    else if(g_comm.state == COMM_STATE_SEND_ECU_INIT)
    {
        /* establish a diagnostics connection with the ECU */
        send_ecu_diag_messages();

        /* start timeout and wait for a response */
        const uint32_t msg_found = comm_wait_for_resp(
                HOBD_MSG_SUBTYPE_INIT,
                1,
                &g_msg_parser,
                &g_comm,
                &rx_timestamp);

        if(msg_found == 1)
        {
            new_hobd_msg_callback(
                    (const hobd_msg_s*) &g_msg_rx_buffer[0],
                    &rx_timestamp);

            g_comm.state = COMM_STATE_SEND_REQ0;
            MODLOGD("->STATE_SEND_REQ0");
        }
        else
        {
            g_comm.state = COMM_STATE_GPIO_INIT;
            MODLOGD("<-STATE_GPIO_INIT");
        }
    }
    else if(g_comm.state == COMM_STATE_SEND_REQ0)
    {
        send_table_req(HOBD_TABLE_10);

        const uint32_t msg_found = comm_wait_for_resp(
                HOBD_MSG_SUBTYPE_TABLE_SUBGROUP,
                1,
                &g_msg_parser,
                &g_comm,
                &rx_timestamp);

        if(msg_found == 1)
        {
            new_hobd_msg_callback(
                    (const hobd_msg_s*) &g_msg_rx_buffer[0],
                    &rx_timestamp);

            g_comm.state = COMM_STATE_SEND_REQ1;
            MODLOGD("->STATE_SEND_REQ1");
        }
        else
        {
            g_comm.state = COMM_STATE_SEND_ECU_INIT;
            MODLOGD("<-STATE_SEND_ECU_INIT");
        }
    }
    else if(g_comm.state == COMM_STATE_SEND_REQ1)
    {
        send_table_req(HOBD_TABLE_D1);

        const uint32_t msg_found = comm_wait_for_resp(
                HOBD_MSG_SUBTYPE_TABLE_SUBGROUP,
                1,
                &g_msg_parser,
                &g_comm,
                &rx_timestamp);

        if(msg_found == 1)
        {
            new_hobd_msg_callback(
                    (const hobd_msg_s*) &g_msg_rx_buffer[0],
                    &rx_timestamp);

            g_comm.state = COMM_STATE_SEND_REQ0;
            MODLOGD("->STATE_SEND_REQ0");
        }
        else
        {
            g_comm.state = COMM_STATE_SEND_ECU_INIT;
            MODLOGD("<-STATE_SEND_ECU_INIT");
        }
    }
}

static void obd_comm_thread_fn(
        const seL4_CPtr ep_cap)
{
    int err;

    /* wait for system ready */
    system_module_wait_for_start();

    /* create a timeout id */
    time_server_alloc_id(&g_comm.timeout_id);

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

    MODLOGD(HOBDMOD_THREAD_NAME " thread is running");

    g_comm.state = COMM_STATE_GPIO_INIT;

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
    thread_set_priority(HOBDMOD_THREAD_PRIORITY, &g_thread);
    thread_set_affinity(HOBDMOD_THREAD_AFFINITY, &g_thread);

    MODLOGD("%s is initialized", HOBDMOD_THREAD_NAME);

    /* start the new thread */
    thread_start(&g_thread);
}
