/**
 * @file main.c
 * @brief TODO.
 *
 */

#include <autoconf.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <sel4/sel4.h>
#include <vka/vka.h>
#include <vka/object.h>
#include <vka/object_capops.h>
#include <allocman/bootstrap.h>
#include <allocman/vka.h>
#include <simple/simple.h>
#include <simple-default/simple-default.h>
#include <sel4platsupport/platsupport.h>
#include <sel4platsupport/io.h>
#include <sel4utils/vspace.h>
#include <sel4utils/thread.h>
#include <sel4utils/page_dma.h>
#include <sel4debug/debug.h>
#include <platsupport/io.h>
#include <utils/io.h>
#include <utils/zf_log.h>
#include <sel4utils/sel4_zf_logif.h>

#include "init_env.h"
#include "platform.h"
#include "root_task.h"
#include "thread.h"
#include "hobd_module.h"

/* 32 * 4K = 128K */
#define MEM_POOL_SIZE ((1 << seL4_PageBits) * 32)

/* dimensions of virtual memory for the allocator to use */
#define ALLOCATOR_VIRTUAL_POOL_SIZE (BIT(seL4_PageBits) * 100)

static char g_mem_pool[MEM_POOL_SIZE];

int main(
        int argc,
        char **argv)
{
    init_env_s env = {0};

    /* initialize the root task */
    root_task_init(
            ALLOCATOR_VIRTUAL_POOL_SIZE,
            MEM_POOL_SIZE,
            &g_mem_pool[0],
            &env);

    /* initialize modules */
    hobd_module_init(&env);

#ifdef CONFIG_DEBUG_BUILD
    ZF_LOGD("Dumping scheduler");
    printf("\n");
    seL4_DebugDumpScheduler();
    printf("\n");
#endif

    /* loop forever, servicing events/faults/etc */
    while(1)
    {
        seL4_Word badge;

        const seL4_MessageInfo_t info = seL4_Recv(
                env.global_fault_ep,
                &badge);

        ZF_LOGD("Received fault on ep 0x%X - badge 0x%X", env.global_fault_ep, badge);

        sel4utils_print_fault_message(info, "fault-handler");
    }

    /* should not get here, intentional halt */
    seL4_DebugHalt();

    return 0;
}
