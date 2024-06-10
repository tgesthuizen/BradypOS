#include <kern/memory.h>
#include <kern/syscall.h>
#include <l4/errors.h>
#include <l4/kip.h>
#include <stddef.h>

extern L4_kip_t the_kip;
extern L4_fpage_t kip_fpage;

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

    // The returned control is always 0
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
