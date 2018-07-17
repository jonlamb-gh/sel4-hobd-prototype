/**
 * @file cli.h
 * @brief TODO.
 *
 */

#ifndef CLI_H
#define CLI_H

#include <stdint.h>

#define CLI_IS_CMD_FALSE (0)
#define CLI_IS_CMD_TRUE (1)

typedef enum
{
    CLI_CMD_HELP,
    CLI_CMD_VERSION,
    CLI_CMD_CLEAR,
    CLI_CMD_TIME,
    CLI_CMD_STATS,
    CLI_CMD_MMC_STATE,
    CLI_CMD_MMC_FILE_SIZE,
    CLI_CMD_MMC_RM,
    CLI_CMD_DEBUG_SCHEDULER,
    CLI_CMD_HOBD_COMM_STATE,
    CLI_CMD_HOBD_COMM_LISTEN_ONLY,
    CLI_CMD_KIND_COUNT
} cli_cmd_kind;

typedef struct
{
    const char *cmd;
    const char *desc;
} cli_cmd_desc_s;

const cli_cmd_desc_s *cli_get_cmd_desc(
        const cli_cmd_kind cmd);

const char *cli_get_cmd_str(
        const cli_cmd_kind cmd);

const char *cli_get_cmd_desc_str(
        const cli_cmd_kind cmd);

/* 0 if not a command */
int cli_is_cmd(
        const char * const str,
        cli_cmd_kind * const cmd);

#endif /* CLI_H */
