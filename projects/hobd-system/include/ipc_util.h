/**
 * @file ipc_util.h
 * @brief TODO.
 *
 */

#ifndef IPC_UTIL_H
#define IPC_UTIL_H

#include <stdint.h>

/* max is 0xFF for base and offset */
#define IPC_MSG_TYPE_ID(base, id) ((base << 8) + id)

#define IPC_ENDPOINT_BADGE(base) (base)

#define IPC_FAULT_ENDPOINT_BADGE(base) ((base << 4) + 1)

#define IPC_NOTIFICATION_BADGE(base) ((base << 4) + 0xA)

#endif /* IPC_UTIL_H */
