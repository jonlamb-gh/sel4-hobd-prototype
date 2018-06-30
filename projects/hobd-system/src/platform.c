/**
 * @file platform.c
 * @brief TODO.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <sel4/sel4.h>
#include <sel4debug/debug.h>
#include <sel4platsupport/platsupport.h>
#include <sel4platsupport/io.h>
#include <sel4utils/vspace.h>
#include <sel4utils/page_dma.h>
#include <sel4debug/debug.h>
#include <platsupport/io.h>
#include <platsupport/clock.h>
#include <platsupport/delay.h>
#include <utils/frequency.h>

#include "config.h"
#include "init_env.h"
#include "platform.h"

#define IMX6_MAX_FREQ (996 * MHZ)

static void init_clock(
        init_env_s * const env)
{
    int err;
    clock_sys_t clock = {0};

    err = clock_sys_init(&env->io_ops, &clock);
    ZF_LOGF_IF(err != 0, "Failed to initialize clock");

    clk_t * const clock_ref = clk_get_clock(&clock, CLK_ARM);
    ZF_LOGF_IF(clock_ref == NULL, "Failed to get CLK_ARM clock");

    /* set to highest cpu freq */
    const freq_t freq = clk_set_freq(clock_ref, IMX6_MAX_FREQ);
    ZF_LOGF_IF(freq != IMX6_MAX_FREQ, "Failed to set imx6 clock frequency");
}

void platform_init(
        init_env_s * const env)
{
    int err;

    /* Setup debug port: printf() is only reliable after this */
    platsupport_serial_setup_simple(
            &env->vspace,
            &env->simple,
            &env->vka);

    /* initialize platform I/O operations data */
    err = sel4platsupport_new_io_ops(
            env->vspace,
            env->vka,
            &env->io_ops);
    ZF_LOGF_IF(err != 0, "Failed to create new IO ops\n");

    /* initialize architecture I/O operations data */
    err = sel4platsupport_new_arch_ops(
            &env->io_ops,
            &env->simple,
            &env->vka);
    ZF_LOGF_IF(err != 0, "Failed to initialize IO ops\n");

    /* create a new I/O mapper which will be used to get the frame(s) */
    err = sel4platsupport_new_io_mapper(
            env->vspace,
            env->vka,
            &env->io_ops.io_mapper);
    ZF_LOGF_IF(err != 0, "Failed to create new IO mapper\n");

    /* create new malloc ops */
    err = sel4platsupport_new_malloc_ops(
            &env->io_ops.malloc_ops);
    ZF_LOGF_IF(err != 0, "Failed to create new malloc ops\n");

    /* create a dma manager, allocates at page granularity */
    err = sel4utils_new_page_dma_alloc(
            &env->vka,
            &env->vspace,
            &env->io_ops.dma_manager);
    ZF_LOGF_IF(err != 0, "Failed to create new DMA manager\n");

    /* initialize clock, set max frequency */
    init_clock(env);

    /* inform libplatsupport about our CPU frequency */
    ps_cpufreq_hint(IMX6_MAX_FREQ);

    ZF_LOGD("Platform is initialized");
}
