/**
 * @file sel4_mr.h
 * @brief TODO.
 *
 * TODO - use memcpy()
 *
 */

#ifndef SEL4_MR_H
#define SEL4_MR_H

#include <stdint.h>

#include <sel4/sel4.h>
#include <vka/object.h>
#include <vka/object_capops.h>

static inline void sel4_mr_recv(
        const seL4_IPCBuffer * const ipc,
        const uint32_t size_words,
        uint32_t * const buffer)
{
    uint32_t idx;

    for(idx = 0; idx < size_words; idx += 1)
    {
        /* buffer[idx] = (uint32_t) ipc->msg[idx]; */

        if(idx <= 3)
        {
            buffer[idx] = (uint32_t) seL4_GetMR(idx);
        }
        else
        {
            buffer[idx] = (uint32_t) ipc->msg[idx];
        }
    }
}

/* TODO - what am I doing wrong here to require this? */
static inline void sel4_mr_send(
        const uint32_t size_words,
        const uint32_t * const buffer,
        seL4_IPCBuffer * const ipc)
{
    uint32_t idx;

    for(idx = 0; idx < size_words; idx += 1)
    {
        if(idx <= 3)
        {
            seL4_SetMR(idx, (seL4_Word) buffer[idx]);
        }
        else
        {
            ipc->msg[idx] = (seL4_Word) buffer[idx];
        }
    }
}

#endif /* SEL4_MR_H */
