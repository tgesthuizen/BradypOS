#ifndef BRADYPOS_KERN_MEMORY_H
#define BRADYPOS_KERN_MEMORY_H

#include <l4/space.h>
#include <stdint.h>

enum mempool_type
{
    MEMPOOL_RAM,
    MEMPOOL_FAST_RAM,
    MEMPOOL_MEMIO,
    MEMPOOL_UNKNOWN,
};

enum mempool_flags
{
    MP_R = 1 << 0,
    MP_W = 1 << 1,
    MP_X = 1 << 2,
};

struct mempool
{
    const char *name;
    uintptr_t start;
    uintptr_t end;

    enum mempool_type type;
    // Populated with values from enum mempool_flags
    unsigned flags;
};

/**
 * @brief Contains all memory pools
 *
 */
extern const struct mempool mempools[];

struct tcb_t;

struct as_t
{
    unsigned ref_count;
    L4_fpage_t utcb_page;
    struct tcb_t *first_thread;
};

void init_memory();
struct as_t *create_as(struct tcb_t *owner);

#endif
