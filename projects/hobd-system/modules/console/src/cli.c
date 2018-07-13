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

static const cli_cmd_desc_s CMD_DESCRIPTORS[] =
{
    [CLI_CMD_HELP] = {   "help", "    - print this help message"},
    [CLI_CMD_VERSION] = {"version", " - print version information"},
    [CLI_CMD_CLEAR] = {  "clear", "   - clear the scren"},
    [CLI_CMD_TIME] = {   "time", "    - get the current time"},
    [CLI_CMD_STATS] = {  "stats", "   - print module statistics and metrics"},
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
