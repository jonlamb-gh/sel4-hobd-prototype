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

#define CLI_IS_SUBCMD_FALSE (0)
#define CLI_IS_SUBCMD_TRUE (1)
#define CLI_IS_SUBCMD_MISSING (2)

typedef enum
{
    CLI_CMD_HELP = 0,
    CLI_CMD_VERSION,
    CLI_CMD_CLEAR,
    CLI_CMD_TIME,
    CLI_CMD_INFO,
    CLI_CMD_DEBUG_SCHEDULER,
    CLI_CMD_MMC,
    CLI_CMD_HOBD,
    CLI_CMD_KIND_COUNT
} cli_cmd_kind;

typedef enum
{
    CLI_MMC_SUBCMD_ON = 0,
    CLI_MMC_SUBCMD_OFF,
    CLI_MMC_SUBCMD_STATUS,
    CLI_MMC_SUBCMD_FILE_SIZE,
    CLI_MMC_SUBCMD_RM,
    CLI_MMC_SUBCMD_KIND_COUNT
} cli_mmc_subcmd_kind;

typedef enum
{
    CLI_HOBD_SUBCMD_ON = 0,
    CLI_HOBD_SUBCMD_OFF,
    CLI_HOBD_SUBCMD_STATUS,
    CLI_HOBD_SUBCMD_PASSIVE,
    CLI_HOBD_SUBCMD_KIND_COUNT
} cli_hobd_subcmd_kind;

typedef struct
{
    const char *subcmd;
    const char *desc;
} cli_subcmd_desc_s;

typedef struct
{
    const char *cmd;
    const char *desc;
    const cli_subcmd_desc_s *subcmds;
} cli_cmd_desc_s;

const cli_cmd_desc_s *cli_get_cmd_desc(
        const cli_cmd_kind cmd);

const char *cli_get_cmd_str(
        const cli_cmd_kind cmd);

const char *cli_get_cmd_desc_str(
        const cli_cmd_kind cmd);

const cli_subcmd_desc_s *cli_get_subcmd_array(
        const cli_cmd_kind cmd);

uint32_t cli_get_subcmd_count(
        const cli_cmd_kind cmd);

/* 0 if not a command */
int cli_is_cmd(
        const char * const str,
        cli_cmd_kind * const cmd);

/* 0 if not a subcommand of command */
int cli_is_subcmd(
        const char * const str,
        const cli_cmd_kind cmd,
        uint32_t * const subcmd_enum);

#endif /* CLI_H */
