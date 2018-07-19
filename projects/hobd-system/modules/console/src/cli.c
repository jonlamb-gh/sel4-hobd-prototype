/**
 * @file cli.c
 * @brief TODO.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <sel4/sel4.h>
#include <vka/vka.h>
#include <sel4utils/sel4_zf_logif.h>
#include <sel4debug/debug.h>

#include "config.h"
#include "cli.h"

#ifdef CONSOLEMOD_DEBUG
#define MODLOGD(...) ZF_LOGD(__VA_ARGS__)
#else
#define MODLOGD(...)
#endif

#define CMD_INITIALIZER(id, cmd, desc, subcmds) [id] = {cmd, desc, subcmds}
#define SUBCMD_INITIALIZER(id, subcmd, desc) [id] = {subcmd, desc}

static const cli_subcmd_desc_s MMC_SUBCMD_DESCRIPTORS[] =
{
    SUBCMD_INITIALIZER(
            CLI_MMC_SUBCMD_ON,
            "on", "         - enable MMC module file logging"),
    SUBCMD_INITIALIZER(
            CLI_MMC_SUBCMD_OFF,
            "off", "        - disable MMC module file logging"),
    SUBCMD_INITIALIZER(
            CLI_MMC_SUBCMD_STATUS,
            "status", "     - get MMC module status"),
    SUBCMD_INITIALIZER(
            CLI_MMC_SUBCMD_FILE_SIZE,
            "size", "       - get the curent MMC file size"),
    SUBCMD_INITIALIZER(
            CLI_MMC_SUBCMD_RM,
            "rm", "         - delete the current MMC file"),
    {NULL, NULL}
};

static const cli_subcmd_desc_s HOBD_SUBCMD_DESCRIPTORS[] =
{
    SUBCMD_INITIALIZER(
            CLI_HOBD_SUBCMD_ON,
            "on", "         - enable HOBD K-line comms"),
    SUBCMD_INITIALIZER(
            CLI_HOBD_SUBCMD_OFF,
            "off", "        - disable HOBD K-line comms"),
    SUBCMD_INITIALIZER(
            CLI_HOBD_SUBCMD_STATUS,
            "status", "     - get HOBD comms module status"),
    SUBCMD_INITIALIZER(
            CLI_HOBD_SUBCMD_PASSIVE,
            "passive", "    - toggle HOBD comms listen-only mode"),
    {NULL, NULL}
};

static const cli_cmd_desc_s CMD_DESCRIPTORS[] =
{
    CMD_INITIALIZER(
            CLI_CMD_HELP,
            "help", "           - print this help message",
            NULL),
    CMD_INITIALIZER(
            CLI_CMD_VERSION,
            "version", "        - print version information",
            NULL),
    CMD_INITIALIZER(
            CLI_CMD_CLEAR,
            "clear", "          - clear the screen",
            NULL),
    CMD_INITIALIZER(
            CLI_CMD_TIME,
            "time", "           - get the current time",
            NULL),
    CMD_INITIALIZER(
            CLI_CMD_INFO,
            "info", "           - print module info, statistics and metrics",
            NULL),
    CMD_INITIALIZER(
            CLI_CMD_DEBUG_SCHEDULER,
            "debug-sched", "    - print the kernel scheduler information",
            NULL),
    CMD_INITIALIZER(
            CLI_CMD_MMC,
            "mmc", " <subcmd>   - MMC module and file ops",
            &MMC_SUBCMD_DESCRIPTORS[0]),
    CMD_INITIALIZER(
            CLI_CMD_HOBD,
            "hobd", " <subcmd>  - HOBD and subsystems",
            &HOBD_SUBCMD_DESCRIPTORS[0])
};

const cli_cmd_desc_s *cli_get_cmd_desc(
        const cli_cmd_kind cmd)
{
    ZF_LOGF_IF(cmd >= CLI_CMD_KIND_COUNT, "Invalid CLI command enum");

    return &CMD_DESCRIPTORS[cmd];
}

const char *cli_get_cmd_str(
        const cli_cmd_kind cmd)
{
    const cli_cmd_desc_s * const desc = cli_get_cmd_desc(cmd);

    return desc->cmd;
}

const char *cli_get_cmd_desc_str(
        const cli_cmd_kind cmd)
{
    const cli_cmd_desc_s * const desc = cli_get_cmd_desc(cmd);

    return desc->desc;
}

const cli_subcmd_desc_s *cli_get_subcmd_array(
        const cli_cmd_kind cmd)
{
    const cli_cmd_desc_s * const desc = cli_get_cmd_desc(cmd);

    return desc->subcmds;
}

uint32_t cli_get_subcmd_count(
        const cli_cmd_kind cmd)
{
    uint32_t cnt = 0;

    const cli_subcmd_desc_s * const subcmds = cli_get_subcmd_array(cmd);

    if(subcmds != NULL)
    {
        while(subcmds[cnt].subcmd != NULL)
        {
            cnt += 1;
        }
    }

    return cnt;
}

int cli_is_cmd(
        const char * const str,
        cli_cmd_kind * const cmd)
{
    int ret = CLI_IS_CMD_FALSE;

    ZF_LOGF_IF((str == NULL) || (cmd == NULL), "Invalid CLI parameters");

    unsigned int idx;
    for(idx = 0; (idx < CLI_CMD_KIND_COUNT) && (ret == CLI_IS_CMD_FALSE); idx += 1)
    {
        const int cmp = strcmp(str, CMD_DESCRIPTORS[idx].cmd);

        if(cmp == 0)
        {
            *cmd = (cli_cmd_kind) idx;
            ret = CLI_IS_CMD_TRUE;
        }
    }

    return ret;
}

int cli_is_subcmd(
        const char * const str,
        const cli_cmd_kind cmd,
        uint32_t * const subcmd_enum)
{
    int ret = CLI_IS_CMD_FALSE;

    ZF_LOGF_IF((str == NULL) || (subcmd_enum == NULL), "Invalid CLI parameters");

    const uint32_t subcmd_cnt = cli_get_subcmd_count(cmd);
    const cli_subcmd_desc_s * const subcmds = cli_get_subcmd_array(cmd);

    uint32_t idx;
    for(idx = 0; (idx < subcmd_cnt) && (subcmds != NULL) && (ret == CLI_IS_CMD_FALSE); idx += 1)
    {
        const int cmp = strcmp(str, subcmds[idx].subcmd);

        if(cmp == 0)
        {
            *subcmd_enum = idx;
            ret = CLI_IS_CMD_TRUE;
        }
    }

    if(subcmd_cnt != 0)
    {
        if(ret == CLI_IS_CMD_FALSE)
        {
            ret = CLI_IS_SUBCMD_MISSING;
        }
    }

    return ret;
}
