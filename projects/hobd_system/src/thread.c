/**
 * @file thread.c
 * @brief TODO.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <sel4/sel4.h>
#include <vspace/vspace.h>
#include <vka/object.h>
#include <vka/object_capops.h>
#include <sel4utils/vspace.h>
#include <sel4utils/mapping.h>
#include <utils/arith.h>
//#include <sel4utils/sel4_zf_logif.h>
#include <sel4debug/debug.h>

#include "init_env.h"
#include "thread.h"

void thread_create(
        const char * const name,
        const seL4_Word ipc_badge,
        const uint32_t stack_size,
        uint64_t * const stack,
        const thread_run_function_type thread_fn,
        init_env_s * const env,
        thread_s * const thread)
{
    int err;

    /* get our vspace root page directory */
    const seL4_CPtr pd_cap = simple_get_pd(&env->simple);

    /* root of the cspace to start the thread in */
    const seL4_CNode cspace_cap = simple_get_cnode(&env->simple);

    /* create a new TCB */
    err = vka_alloc_tcb(&env->vka, &thread->tcb_object);
    ZF_LOGF_IF(err != 0, "Failed to allocate new TCB\n");

    /* get a frame cap for the IPC buffer */
    err = vka_alloc_frame(
            &env->vka,
            THREAD_IPC_BUFFER_FRAME_SIZE_BITS,
            &thread->ipc_frame_object);
    ZF_LOGF_IF(err != 0, "Failed to allocate a frame for the IPC buffer\n");

    /* create a IPC buffer and capability for it */
    seL4_CPtr ipc_pd_cap = 0;
    seL4_IPCBuffer * const ipc_buffer = (seL4_IPCBuffer*) vspace_new_ipc_buffer(
            &env->vspace,
            &ipc_pd_cap);
    assert(ipc_buffer != NULL);

    /* points to itself for now, could be a better wrapper structure */
    ipc_buffer->userData = (seL4_Word) ipc_buffer;

    /* allocate a cspace slot for the fault endpoint */
    seL4_CPtr fault_ep = 0;
    err = vka_cspace_alloc(
            &env->vka,
            &fault_ep);
    ZF_LOGF_IF(err != 0, "Failed to allocate thread fault endpoint\n");

    ZF_LOGD("Minting fault ep 0x%X for thread '%s'", fault_ep, name);

    /* create a badged fault endpoint for the thread */
    err = seL4_CNode_Mint(
            cspace_cap,
            fault_ep,
            seL4_WordBits,
            seL4_CapInitThreadCNode,
            env->global_fault_ep,
            seL4_WordBits,
            seL4_AllRights,
            ipc_badge);
    ZF_LOGF_IF(err != 0, "Failed to mint badged endpoint for thread\n");

    /* initialise the new TCB */
    err = seL4_TCB_Configure(
            thread->tcb_object.cptr,
            fault_ep,
            cspace_cap,
            seL4_NilData,
            pd_cap,
            seL4_NilData,
            (seL4_Word) ipc_buffer,
            thread->ipc_frame_object.cptr);
    ZF_LOGF_IF(err != 0, "Failed to configure new TCB object\n");

#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugNameThread(thread->tcb_object.cptr, name);
#endif

    /* set start up registers for the new thread */
    seL4_UserContext regs = {0};
    const size_t regs_size = sizeof(seL4_UserContext) / sizeof(seL4_Word);

    /* set instruction pointer where the thread will start running */
    sel4utils_set_instruction_pointer(&regs, (seL4_Word) thread_fn);

    /* check that stack is aligned correctly */
    const int stack_alignment_requirement = sizeof(seL4_Word) * 2;
    const uintptr_t thread_stack_top =
            (uintptr_t) &stack[0]
            + stack_size;

    ZF_LOGF_IF(
            thread_stack_top % (stack_alignment_requirement) != 0,
            "Stack top isn't aligned correctly to a %dB boundary",
            stack_alignment_requirement);

    /* set stack pointer for the new thread */
    sel4utils_set_stack_pointer(&regs, thread_stack_top);

    /* write the TCB registers. */
    err = seL4_TCB_WriteRegisters(thread->tcb_object.cptr, 0, 0, regs_size, &regs);
    ZF_LOGF_IF(err != 0, "Failed to write new thread's register set\n");

    ZF_LOGD("Created thread '%s' - stack size %u bytes", name, (size_t) stack_size);
}

void thread_set_priority(
        const int priority,
        thread_s * const thread)
{
    const int err = seL4_TCB_SetPriority(
            thread->tcb_object.cptr,
            seL4_CapInitThreadTCB,
            seL4_MaxPrio);
    ZF_LOGF_IF(err != 0, "Failed to set thread priority\n");
}

void thread_set_affinity(
        const seL4_Word affinity,
        thread_s * const thread)
{
    /* set affinity */
#if CONFIG_MAX_NUM_NODES > 1
    const int err = seL4_TCB_SetAffinity(
            thread->tcb_object.cptr,
            affinity);
    ZF_LOGF_IF(err != 0, "Failed to set thread's affinity\n");
#endif
}

void thread_start(
        thread_s * const thread)
{
    const int err = seL4_TCB_Resume(thread->tcb_object.cptr);
    ZF_LOGF_IF(err != 0, "Failed to start new thread\n");
}
