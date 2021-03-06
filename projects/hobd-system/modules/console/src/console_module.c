/**
 * @file console_module.c
 * @brief TODO.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <sel4/sel4.h>
#include <vka/vka.h>
#include <vka/object.h>
#include <sel4debug/debug.h>
#include <sel4platsupport/device.h>
#include <platsupport/io.h>
#include <platsupport/mux.h>
#include <platsupport/gpio.h>
#include <platsupport/chardev.h>
#include <platsupport/serial.h>
#include <platsupport/delay.h>
#include <sel4utils/sel4_zf_logif.h>

#include "microrl/microrl.h"

#include "config.h"
#include "ipc_util.h"
#include "init_env.h"
#include "thread.h"
#include "time_server.h"
#include "mmc.h"
#include "hobd.h"
#include "cli.h"
#include "console_module.h"

#ifdef CONSOLEMOD_DEBUG
#define MODLOGD(...) ZF_LOGD(__VA_ARGS__)
#else
#define MODLOGD(...)
#endif

#define CONSOLE_BAUD (115200UL)
#define CONSOLE_STARTUP_DELAY_MS (1500UL)

#define ENDPOINT_BADGE IPC_ENDPOINT_BADGE(CONSOLEMOD_BASE_BADGE)

#define NOTIFICATION_BADGE IPC_NOTIFICATION_BADGE(CONSOLEMOD_BASE_BADGE)

static microrl_t g_microrl_state;

static ps_chardevice_t g_cdev;
static vka_object_t g_cdev_ntfn;
static sel4ps_irq_t g_cdev_irq;

static thread_s g_thread;
static uint64_t g_thread_stack[CONSOLEMOD_STACK_SIZE];

/* prototypes */
static void handle_cli_cmd(
        const cli_cmd_kind cmd);
static void handle_cli_subcmd(
        const cli_cmd_kind cmd,
        const uint32_t subcmd_enum);

/* microrl API */
static void console_print(
        const char *str)
{
    uint32_t idx;
    for(idx = 0; (str != NULL) && (str[idx] != 0); idx += 1)
    {
        ps_cdev_putchar(&g_cdev, str[idx]);
    }
}

/* microrl API */
static void console_println(
        const char *str)
{
    console_print(str);
    ps_cdev_putchar(&g_cdev, '\n');
}

/* microrl API */
static int console_exec(
        int argc,
        const char * const * argv)
{
    /* arg0 is the command */
    /* arg1 is an optional command and anything else */
    if(argc > 0)
    {
        cli_cmd_kind cmd;
        const int cmd_status = cli_is_cmd(argv[0], &cmd);

        console_print("\n");

        if(cmd_status == CLI_IS_CMD_TRUE)
        {
            const uint32_t subcmd_cnt = cli_get_subcmd_count(cmd);

            if(subcmd_cnt == 0)
            {
                handle_cli_cmd(cmd);
            }
            else
            {
                if((argc < (int) 2) || (argv[1] == NULL))
                {
                    console_println("Invalid subcommand/arguments");
                }
                else
                {
                    uint32_t subcmd_enum = 0;
                    const int subcmd_status = cli_is_subcmd(argv[1], cmd, &subcmd_enum);

                    if(subcmd_status == CLI_IS_SUBCMD_TRUE)
                    {
                        handle_cli_subcmd(cmd, subcmd_enum);
                    }
                    else
                    {
                        console_println("Invalid subcommand/arguments");
                    }
                }
            }
        }
        else
        {
            console_print(argv[0]);
            console_println(": command not found");
        }
    }

    return 0;
}

/* microrl API */
static void console_sigint_handler(void)
{
    /* TODO */
}

static void print_help(void)
{
    console_println("--- console ---");
    console_println("commands:");

    unsigned int idx;
    for(idx = CLI_CMD_HELP; idx < CLI_CMD_KIND_COUNT; idx += 1)
    {
        console_print(cli_get_cmd_str((cli_cmd_kind) idx));
        console_println(cli_get_cmd_desc_str((cli_cmd_kind) idx));

        const uint32_t subcmd_cnt = cli_get_subcmd_count((cli_cmd_kind) idx);
        const cli_subcmd_desc_s * const subcmds =
                cli_get_subcmd_array((cli_cmd_kind) idx);

        uint32_t subcmd_idx;
        for(subcmd_idx = 0; (subcmd_idx < subcmd_cnt) && (subcmds != NULL); subcmd_idx += 1)
        {
            console_print("    ");
            console_print(subcmds[subcmd_idx].subcmd);
            console_println(subcmds[subcmd_idx].desc);
        }
    }

    console_print("\n");
}

static void print_version(void)
{
    console_println("version information");
    console_println("  branch: " GIT_BRANCH);
    console_println("  revision: " GIT_REVISION);
}

static void clear_console(void)
{
    /* ESC seq for clear entire screen */
    console_print("\033[2J");

    /* ESC seq for move cursor at left-top corner */
    console_print("\033[H");
}

/* TODO - clean this up, move somewhere else */
static void handle_cli_cmd(
        const cli_cmd_kind cmd)
{
    char str[64];

    if(cmd == CLI_CMD_HELP)
    {
        print_help();
    }
    else if(cmd == CLI_CMD_VERSION)
    {
        print_version();
    }
    else if(cmd == CLI_CMD_CLEAR)
    {
        clear_console();
    }
    else if(cmd == CLI_CMD_TIME)
    {
        uint64_t time_ns;
        uint64_t time_sec;
        uint64_t time_min;

        time_server_get_time(&time_ns);

        time_min = (time_ns / NS_IN_MINUTE);
        time_sec = (time_ns - (time_min * NS_IN_MINUTE)) / NS_IN_S;

        console_print("Current time: ");
        (void) snprintf(str, sizeof(str), "%llu", time_ns);
        console_print(str);
        console_println(" ns");

        console_print("Elapsed: ");
        (void) snprintf(str, sizeof(str), "%llu", time_min);
        console_print(str);
        console_print(" min : ");
        (void) snprintf(str, sizeof(str), "%llu", time_sec);
        console_print(str);
        console_println(" sec");
    }
    else if(cmd == CLI_CMD_INFO)
    {
        console_println("Information, Statistics and Metrics");

        mmc_stats_s mmc_stats;
        mmc_get_stats(&mmc_stats);

        console_println("MMC");
        console_print("  timestamp: ");
        (void) snprintf(str, sizeof(str), "%llu", mmc_stats.timestamp);
        console_println(str);
        console_print("  entries_logged: ");
        (void) snprintf(str, sizeof(str), "%u", mmc_stats.entries_logged);
        console_println(str);
        console_print("  enabled: ");
        (void) snprintf(str, sizeof(str), "%u", mmc_get_state());
        console_println(str);

        hobd_stats_s hobd_stats;
        hobd_get_stats(&hobd_stats);

        console_println("HOBD");
        console_print("  timestamp: ");
        (void) snprintf(str, sizeof(str), "%llu", hobd_stats.timestamp);
        console_println(str);
        console_print("  valid_rx_count: ");
        (void) snprintf(str, sizeof(str), "%u", hobd_stats.valid_rx_count);
        console_println(str);
        console_print("  invalid_rx_count: ");
        (void) snprintf(str, sizeof(str), "%u", hobd_stats.invalid_rx_count);
        console_println(str);
        console_print("  comm_gpio_retry_count: ");
        (void) snprintf(str, sizeof(str), "%u", hobd_stats.comm_gpio_retry_count);
        console_println(str);
        console_print("  comm_init_retry_count: ");
        (void) snprintf(str, sizeof(str), "%u", hobd_stats.comm_init_retry_count);
        console_println(str);
        console_print("  comm_enabled: ");
        (void) snprintf(str, sizeof(str), "%u", hobd_get_comm_state());
        console_println(str);
        console_print("  comm_listen_only: ");
        (void) snprintf(str, sizeof(str), "%u", hobd_get_listen_only());
        console_println(str);
    }
    else if(cmd == CLI_CMD_DEBUG_SCHEDULER)
    {
#ifdef CONFIG_DEBUG_BUILD
        console_println("Dumping scheduler (only core 0 TCBs will be displayed?)");
        console_print("\n");
        seL4_DebugDumpScheduler();
        console_print("\n");
#else
        console_println("Must be a debug build to do so");
#endif
    }
    else
    {
        console_print(cli_get_cmd_str(cmd));
        console_println(": not yet supported");
    }
}

static void handle_cli_subcmd(
        const cli_cmd_kind cmd,
        const uint32_t subcmd_enum)
{
    char str[64];

    if(cmd == CLI_CMD_MMC)
    {
        const cli_mmc_subcmd_kind subcmd = (cli_mmc_subcmd_kind) subcmd_enum;

        if(subcmd == CLI_MMC_SUBCMD_ON)
        {
            console_println("Enabling MMC file");

            const uint32_t state = mmc_set_state(1);

            console_print("MMC status: ");
            (void) snprintf(str, sizeof(str), "%u", state);
            console_println(str);
        }
        else if(subcmd == CLI_MMC_SUBCMD_OFF)
        {
            console_println("Disabling MMC file");

            const uint32_t state = mmc_set_state(0);

            console_print("MMC status: ");
            (void) snprintf(str, sizeof(str), "%u", state);
            console_println(str);
        }
        else if(subcmd == CLI_MMC_SUBCMD_STATUS)
        {
            console_print("MMC status: ");
            (void) snprintf(str, sizeof(str), "%u", mmc_get_state());
            console_println(str);
        }
        else if(subcmd == CLI_MMC_SUBCMD_FILE_SIZE)
        {
            uint32_t file_size;

            const int status = mmc_get_file_size(&file_size);

            if(status == 0)
            {
                console_print("MMC file size: ");
                (void) snprintf(str, sizeof(str), "%u", file_size);
                console_print(str);
                console_println(" bytes");
            }
            else
            {
                console_println("Failed to get MMC file size");
            }
        }
        else if(subcmd == CLI_MMC_SUBCMD_RM)
        {
            const int status = mmc_rm();

            if(status == 0)
            {
                console_println("Deleted the MMC file");
            }
            else
            {
                console_println("Failed to delete the MMC file");
            }
        }
    }
    else if(cmd == CLI_CMD_HOBD)
    {
        const cli_hobd_subcmd_kind subcmd = (cli_hobd_subcmd_kind) subcmd_enum;

        if(subcmd == CLI_HOBD_SUBCMD_ON)
        {
            console_println("Enabling HOBD comms");

            const uint32_t state = hobd_set_comm_state(1);

            console_print("HOBD comms status: ");
            (void) snprintf(str, sizeof(str), "%u", state);
            console_println(str);
        }
        else if(subcmd == CLI_HOBD_SUBCMD_OFF)
        {
            console_println("Disabling HOBD comms");

            const uint32_t state = hobd_set_comm_state(0);

            console_print("HOBD comms status: ");
            (void) snprintf(str, sizeof(str), "%u", state);
            console_println(str);
        }
        else if(subcmd == CLI_HOBD_SUBCMD_STATUS)
        {
            console_print("HOBD comms status: ");
            (void) snprintf(str, sizeof(str), "%u", hobd_get_comm_state());
            console_println(str);

            console_print("HOBD comms listen-only: ");
            (void) snprintf(str, sizeof(str), "%u", hobd_get_listen_only());
            console_println(str);
        }
        else if(subcmd == CLI_HOBD_SUBCMD_PASSIVE)
        {
            const uint32_t state = hobd_get_listen_only();

            if(state == 0)
            {
                console_println("Enabling listen-only mode");
            }
            else
            {
                console_println("Disabling listen-only mode");
            }

            const uint32_t new_state = hobd_set_listen_only(!state);

            console_print("HOBD comms listen-only: ");
            (void) snprintf(str, sizeof(str), "%u", new_state);
            console_println(str);
        }
    }
}

static void console_thread_fn(
        const seL4_CPtr ep_cap)
{
    int err;

    MODLOGD(CONSOLEMOD_THREAD_NAME " thread is running");

    ps_mdelay(CONSOLE_STARTUP_DELAY_MS);

    /* reconfigure the serial port */
    err = serial_configure(
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
        /* wait on the IRQ notification */
        seL4_Word mbadge = 0;
        seL4_Wait(g_cdev_ntfn.cptr, &mbadge);
        ZF_LOGF_IF(mbadge != NOTIFICATION_BADGE, "Invalid badge 0x%X", mbadge);

        /* ack the IRQ */
        err = seL4_IRQHandler_Ack(g_cdev_irq.handler_path.capPtr);
        ZF_LOGF_IF(err != 0, "Failed to ack IRQ");

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
    int err;

    /* initialize character device - PS_SERIAL1 = IMX_UART2 - sabre lite console */
    const ps_chardevice_t * const char_dev = ps_cdev_init(
            PS_SERIAL1,
            &env->io_ops,
            &g_cdev);
    ZF_LOGF_IF(char_dev == NULL, "Failed to initialize character device");

    /* construct a IRQ object to get notifications on */
    g_cdev_irq.irq.type = PS_INTERRUPT;
    g_cdev_irq.irq.irq.number = UART2_IRQ;

    /* create a notification object for the IRQ */
    err = vka_alloc_notification(&env->vka, &g_cdev_ntfn);
    ZF_LOGF_IF(err != 0, "Failed to create notification object");

    /* allocate a cslot for an irq and get the cap for an irq */
    err = sel4platsupport_copy_irq_cap(
            &env->vka,
            &env->simple,
            &g_cdev_irq.irq,
            &g_cdev_irq.handler_path);
    ZF_LOGF_IF(err != 0, "Failed to copy IRQ caps");

    /* allocate a cspace slot */
    err = vka_cspace_alloc_path(
            &env->vka,
            &g_cdev_irq.badged_ntfn_path);
    ZF_LOGF_IF(err != 0, "Failed to create IRQ cspace slot");

    /* allocate a cspacepath for the capability */
    cspacepath_t path = {0};
    vka_cspace_make_path(
            &env->vka,
            g_cdev_ntfn.cptr,
            &path);

    /* badge it */
    err = vka_cnode_mint(
            &g_cdev_irq.badged_ntfn_path,
            &path,
            seL4_AllRights,
            NOTIFICATION_BADGE);
    ZF_LOGF_IF(err != 0, "Failed to badge IRQ notification");

    /* set notification acking any pending IRQ to ensure
     * there is no race where we lose an IRQ */
    err = seL4_IRQHandler_SetNotification(
            g_cdev_irq.handler_path.capPtr,
            g_cdev_irq.badged_ntfn_path.capPtr);
    ZF_LOGF_IF(err != 0, "Failed to set IRQ notification");

    /* formally ack it */
    err = seL4_IRQHandler_Ack(g_cdev_irq.handler_path.capPtr);
    ZF_LOGF_IF(err != 0, "Failed to ack IRQ");
}

void console_module_init(
        init_env_s * const env)
{
    init_cdev(env);

    /* create a worker thread */
    thread_create(
            CONSOLEMOD_THREAD_NAME,
            ENDPOINT_BADGE,
            (uint32_t) sizeof(g_thread_stack),
            &g_thread_stack[0],
            &console_thread_fn,
            env,
            &g_thread);

    /* bind timer notification to TCB */
    const int err = seL4_TCB_BindNotification(
            g_thread.tcb_object.cptr,
            g_cdev_ntfn.cptr);
    ZF_LOGF_IF(err != 0, "Failed to bind notification to thread TCB");

    /* set thread priority and affinity */
    thread_set_priority(CONSOLEMOD_THREAD_PRIORITY, &g_thread);
    thread_set_affinity(CONSOLEMOD_THREAD_AFFINITY, &g_thread);

    MODLOGD("%s is initialized", CONSOLEMOD_THREAD_NAME);

    /* start the new thread */
    thread_start(&g_thread);
}
