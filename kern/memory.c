#include <kern/memory.h>
#include <kern/thread.h>
#include <l4/errors.h>
#include <l4/kip.h>
#include <l4/space.h>
#include <l4/utcb.h>
#include <stddef.h>

extern kip_t the_kip;
static L4_fpage_t kip_fpage;

void init_memory()
{
    kip_fpage.b = (unsigned)&the_kip / 512;
    kip_fpage.s = 10;
    kip_fpage.perm = L4_readable;
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
