/**
 * @file ecu.h
 * @brief TODO.
 *
 */

#ifndef ECU_H
#define ECU_H

struct ecu_s;
typedef struct ecu_s ecu_s;

ecu_s *ecu_new(
        const int socket_fd);

int ecu_update(
        ecu_s * const ecu);

#endif /* ECU_H */
