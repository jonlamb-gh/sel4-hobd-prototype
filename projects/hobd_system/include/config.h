/**
 * @file config.h
 * @brief TODO.
 *
 * - STACK_SIZE = size of the thread's stack in words
 * - EP_BADGE = arbitrary (but unique) number for a badge
 * - AFFINITY - CPU core to run on
 *
 */

/* TODO - add thread priority */

#ifndef CONFIG_H
#define CONFIG_H

/* 32 * 4K = 128K */
#define MEM_POOL_SIZE ((1 << seL4_PageBits) * 32)

/* dimensions of virtual memory for the allocator to use */
#define ALLOCATOR_VIRTUAL_POOL_SIZE (BIT(seL4_PageBits) * 100)

#define ROOT_THREAD_NAME "init"
#define ROOT_THREAD_AFFINITY (0)

#define SYSMOD_THREAD_NAME "sys"
#define SYSMOD_THREAD_AFFINITY (0)
#define SYSMOD_STACK_SIZE (512)
#define SYSMOD_EP_BADGE (0x20)

#define HOBDMOD_THREAD_NAME "hobd"
#define HOBDMOD_THREAD_AFFINITY (0)
#define HOBDMOD_STACK_SIZE (512)
#define HOBDMOD_EP_BADGE (0x21)

#endif /* CONFIG_H */
