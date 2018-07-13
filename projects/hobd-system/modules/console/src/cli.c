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

static const char * const CMD_STRINGS[] =
{
    [CLI_CMD_HELP] = "help",
    [CLI_CMD_VERSION] = "version",
    [CLI_CMD_CLEAR] = "clear",
    NULL
};

const char *cli_get_cmd_str(
        const cli_cmd_kind cmd)
{
    ZF_LOGF_IF(cmd >= CLI_CMD_KIND_COUNT, "Invalid CLI command enum");

    return CMD_STRINGS[cmd];
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
        const int cmp = strcmp(str, CMD_STRINGS[idx]);

        if(cmp == 0)
        {
            *cmd = (cli_cmd_kind) idx;
            ret = CLI_IS_CMD_TRUE;
        }
    }

    return ret;
}
