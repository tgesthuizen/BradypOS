#include <kern/debug.h>
#include <kern/memory.h>
#include <kern/thread.h>
#include <l4/errors.h>
#include <l4/kip.h>
#include <l4/space.h>
#include <l4/utcb.h>
#include <stddef.h>

extern kip_t the_kip;
static L4_fpage_t kip_fpage;

enum
{
    MAX_ADDRESS_SPACES = 16
};

static struct as_t as_pool[MAX_ADDRESS_SPACES];
static unsigned as_allocated;

void init_memory()
{
    kip_fpage.b = (unsigned)&the_kip / 512;
    kip_fpage.s = 10;
    kip_fpage.perm = L4_readable;
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

extern struct tcb_t *caller;

void syscall_space_control()
{
    unsigned *sp = (unsigned *)caller->ctx.sp;
    L4_thread_id space_specifier = sp[THREAD_CTX_STACK_R0];
    unsigned control = sp[THREAD_CTX_STACK_R1];
    L4_fpage_t kip_area;
    kip_area.raw = sp[THREAD_CTX_STACK_R2];
    L4_fpage_t utcb_area;
    utcb_area.raw = sp[THREAD_CTX_STACK_R3];
    L4_thread_id redirector = caller->ctx.r[THREAD_CTX_R4];

    // THe returned control is always 0
    sp[THREAD_CTX_STACK_R1] = 0;

    struct tcb_t *target_thread = find_thread_by_global_id(space_specifier);
    if (target_thread == NULL)
    {
        sp[THREAD_CTX_STACK_R0] = 0;
        caller->utcb->error = L4_error_invalid_space;
        return;
    }
    if (kip_area.raw != kip_fpage.raw)
    {
        sp[THREAD_CTX_STACK_R0] = 0;
        caller->utcb->error = L4_error_kip_area;
        return;
    }
    if (utcb_area.s < 9)
    {
        sp[THREAD_CTX_STACK_R0] = 0;
        caller->utcb->error = L4_error_utcb_area;
        return;
    }

    if (target_thread->as == NULL)
    {
        struct as_t *new_as = create_as(target_thread);
        target_thread->next_sibling = target_thread;
        target_thread->prev_sibling = target_thread;
        new_as->utcb_page = L4_nilpage;
        target_thread->as = new_as;
    }

    if (target_thread->as->utcb_page.raw != 0)
    {
        // The error here really is that the address space was already
        // initialized
        sp[THREAD_CTX_STACK_R0] = 0;
        caller->utcb->error = L4_error_utcb_area;
    }

    target_thread->as->utcb_page = utcb_area;

    sp[THREAD_CTX_STACK_R0] = 1;
    sp[THREAD_CTX_STACK_R1] = 0;
    return;
}
