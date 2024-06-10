#include <kern/debug.h>
#include <kern/memory.h>
#include <kern/thread.h>
#include <l4/errors.h>
#include <l4/kip.h>
#include <l4/space.h>
#include <l4/utcb.h>
#include <stddef.h>

extern L4_kip_t the_kip;
L4_fpage_t kip_fpage;

enum
{
    MAX_ADDRESS_SPACES = 16
};

static struct as_t as_pool[MAX_ADDRESS_SPACES];
static unsigned as_allocated;

void init_memory()
{
    kip_fpage =
        L4_fpage_add_rights(L4_fpage_log2((unsigned)&the_kip, 10), L4_readable);
    as_allocated = 0;
}

static unsigned alloc_as()
{
    unsigned i;
    for (i = 0; i < MAX_ADDRESS_SPACES; ++i)
    {
        if (!(as_allocated & (1 << i)))
        {
            as_allocated |= (1 << i);
            break;
        }
    }
    return i;
}

struct as_t *create_as(struct tcb_t *owner)
{
    unsigned as_idx = alloc_as();
    if (as_idx == MAX_ADDRESS_SPACES)
    {
        panic("Could not allocate new address space");
    }
    struct as_t *result = &as_pool[as_idx];
    result->first_thread = owner;
    result->utcb_page = L4_nilpage;
    result->ref_count = 1;
    return result;
}
