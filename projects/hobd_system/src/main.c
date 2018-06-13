#include <autoconf.h>

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <allocman/bootstrap.h>
#include <allocman/vka.h>
#include <simple/simple.h>
#include <simple-default/simple-default.h>
#include <sel4debug/debug.h>

#include <sel4platsupport/platsupport.h>
#include <sel4platsupport/io.h>

#include <sel4utils/vspace.h>
#include <sel4utils/page_dma.h>
#include <platsupport/io.h>

#include <utils/io.h>

static seL4_BootInfo *info;
static simple_t simple;
static vka_t vka;
static allocman_t *allocman;
static vspace_t vspace;

static sel4utils_alloc_data_t alloc_data;

#define MEM_POOL_SIZE ((1 << seL4_PageBits) * 32)
static char mem_pool[MEM_POOL_SIZE];

int main(int argc, char **argv)
{
    int error;

    /* get boot info */
    info = platsupport_get_bootinfo();
    assert(info != NULL);

    /* name this thread */
    seL4_DebugNameThread(seL4_CapInitThreadTCB, "hobd-system");

    /* init simple */
    simple_default_init_bootinfo(&simple, info);

    /* print out bootinfo and other info about simple */
    simple_print(&simple);

    /* create an allocator */
    allocman = bootstrap_use_current_simple(
            &simple,
            MEM_POOL_SIZE,
            mem_pool);
    assert(allocman != NULL);

    /* create a vka (interface for interacting with the underlying allocator) */
    allocman_make_vka(&vka, allocman);

    /* create a vspace object to manage our vspace */
    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(
            &vspace,
            &alloc_data,
            // ./kernel/libsel4/include/sel4/bootinfo_types.h
            //seL4_CapInitThreadPD,
            simple_get_pd(&simple),
            &vka,
            info);
    assert(error == 0);

    printf("\n\nhello from seL4 - halting now\n\n");

    /* intentional halt */
    seL4_DebugHalt();

    return 0;
}
