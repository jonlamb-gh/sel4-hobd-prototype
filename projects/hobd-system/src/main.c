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
#include <platsupport/delay.h>
#include <utils/io.h>
#include <utils/zf_log.h>
#include <sel4utils/sel4_zf_logif.h>

#include "config.h"
#include "init_env.h"
#include "platform.h"
#include "root_task.h"
#include "thread.h"
#include "time_server_module.h"
#include "mmc_module.h"
#include "mmc.h"
#include "console_module.h"
#include "system_module.h"
#include "hobd_module.h"

static char g_mem_pool[MEM_POOL_SIZE];

int main(
        int argc,
        char **argv)
{
    int err;
    init_env_s env = {0};

    /* initialize the root task */
    root_task_init(
            ALLOCATOR_VIRTUAL_POOL_SIZE,
            MEM_POOL_SIZE,
            &g_mem_pool[0],
            &env);

    /* initialize modules, not all of them will wait for the system module to start */
    time_server_module_init(&env);
    mmc_module_init(&env);
    console_module_init(&env);
    hobd_module_init(&env);
    system_module_init(&env);

    err = seL4_TCB_SetPriority(
            seL4_CapInitThreadTCB,
            seL4_CapInitThreadTCB,
            ROOT_THREAD_PRIORITY);
    ZF_LOGF_IF(err != 0, "Failed to set root thread priority");

    err = seL4_TCB_SetAffinity(
            seL4_CapInitThreadTCB,
            ROOT_THREAD_AFFINITY);
    ZF_LOGF_IF(err != 0, "Failed to set root thread's affinity");

    /* loop forever, servicing events/faults/etc */
    while(1)
    {
        seL4_Word badge;

        const seL4_MessageInfo_t info = seL4_Recv(
                env.global_fault_ep,
                &badge);

        ZF_LOGD("-------------------");
        ZF_LOGD("Received fault on ep 0x%X - badge 0x%X", env.global_fault_ep, badge);
        ZF_LOGD("-------------------");

        sel4utils_print_fault_message(info, "fault-handler");

        /* log a fault entry, non-blocking true so it could be dropped */
        mmc_log_entry_data(
                MMC_ENTRY_TYPE_FAULT,
                0,
                NULL,
                NULL,
                1);
    }

    /* should not get here, intentional halt */
    seL4_DebugHalt();

    return 0;
}
