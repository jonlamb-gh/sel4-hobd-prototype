/**
 * @file root_task.h
 * @brief TODO.
 *
 */

#ifndef ROOT_TASK_H
#define ROOT_TASK_H

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
} root_task_s;

void root_task_init(
        const uint32_t mem_pool_size,
        char * const mem_pool,
        root_task_s * const root_task);

#endif /* ROOT_TASK_H */
