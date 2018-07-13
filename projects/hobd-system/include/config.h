/**
 * @file config.h
 * @brief TODO.
 *
 * - STACK_SIZE = size of the thread's stack in words
 * - EP_BADGE = arbitrary (but unique) number for a badge
 * - AFFINITY - CPU core to run on
 *
 *   TODO:
 *   - use CMake configuration options
 *   - rebalance affinity and priority
 *
 */

#ifndef CONFIG_H
#define CONFIG_H

/* TODO */
#define SIMULATION_BUILD

/* TODO - make this bytes? currently uint64's */
#define DEFAULT_STACK_SIZE (512UL)

#define DEFAULT_THREAD_PRIORITY (seL4_MaxPrio)

/* 32 * 4K = 128K */
#define MEM_POOL_SIZE ((1 << seL4_PageBits) * 32UL)

/* dimensions of virtual memory for the allocator to use */
#define ALLOCATOR_VIRTUAL_POOL_SIZE (BIT(seL4_PageBits) * 100UL)

#define ROOT_THREAD_NAME "init"
#define ROOT_THREAD_AFFINITY (0)
#define ROOT_THREAD_PRIORITY (seL4_MaxPrio)

#define TMSERVERMOD_THREAD_NAME "time-server"
#define TMSERVERMOD_THREAD_AFFINITY (1)
#define TMSERVERMOD_THREAD_PRIORITY DEFAULT_THREAD_PRIORITY
#define TMSERVERMOD_STACK_SIZE DEFAULT_STACK_SIZE
#define TMSERVERMOD_EP_BADGE (0x20)

#define SYSMOD_THREAD_NAME "sys"
#define SYSMOD_THREAD_AFFINITY (0)
#define SYSMOD_THREAD_PRIORITY DEFAULT_THREAD_PRIORITY
#define SYSMOD_STACK_SIZE DEFAULT_STACK_SIZE
#define SYSMOD_EP_BADGE (0x21)

#define MMCMOD_THREAD_NAME "mmc"
#define MMCMOD_THREAD_AFFINITY (2)
#define MMCMOD_THREAD_PRIORITY DEFAULT_THREAD_PRIORITY
#define MMCMOD_STACK_SIZE DEFAULT_STACK_SIZE
#define MMCMOD_EP_BADGE (0x22)

#define HOBDMOD_THREAD_NAME "hobd-comm"
#define HOBDMOD_THREAD_AFFINITY (3)
#define HOBDMOD_THREAD_PRIORITY DEFAULT_THREAD_PRIORITY
#define HOBDMOD_STACK_SIZE (2 * DEFAULT_STACK_SIZE)
#define HOBDMOD_EP_BADGE (0x23)
#define HOBDMOD_NOTIFICATION_BADGE (0x23A)

#define CONSOLEMOD_THREAD_NAME "console"
#define CONSOLEMOD_THREAD_AFFINITY (0)
#define CONSOLEMOD_THREAD_PRIORITY DEFAULT_THREAD_PRIORITY
#define CONSOLEMOD_STACK_SIZE (2 * DEFAULT_STACK_SIZE)
#define CONSOLEMOD_EP_BADGE (0x24)
#define CONSOLE_NOTIFICATION_BADGE (0x24A)

/* module specific verbose logging */
/*
#ifdef CONFIG_DEBUG_BUILD
#define TMSERVERMOD_DEBUG
#define SYSMOD_DEBUG
#define MMCMOD_DEBUG
#define HOBDMOD_DEBUG
#define CONSOLEMOD_DEBUG
#endif
*/

#endif /* CONFIG_H */
