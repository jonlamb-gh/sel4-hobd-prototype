/**
 * @file init_env.h
 * @brief TODO.
 *
 */

#ifndef INIT_ENV_H
#define INIT_ENV_H

#include <stdint.h>

#include <sel4/bootinfo_types.h>
#include <simple/simple.h>
#include <allocman/vka.h>
#include <allocman/allocman.h>
#include <sel4utils/vspace.h>

typedef struct
{
    seL4_BootInfo *boot_info;
    simple_t simple;
    vka_t vka;
    allocman_t *allocman;
    vspace_t vspace;
    sel4utils_alloc_data_t alloc_data;
    reservation_t virt_reservation;
    vka_object_t global_fault_ep_obj;
    seL4_CPtr global_fault_ep;
    ps_io_ops_t io_ops;
} init_env_s;

#endif /* INIT_ENV_H */
