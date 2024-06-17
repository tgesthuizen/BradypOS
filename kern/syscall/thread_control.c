#include <kern/debug.h>
#include <kern/memory.h>
#include <kern/syscall.h>
#include <kern/systhread.h>
#include <kern/thread.h>
#include <l4/errors.h>
#include <l4/space.h>
#include <l4/thread.h>
#include <stddef.h>

static bool fpage_contains_object(L4_fpage_t page, void *object_base,
                                  size_t object_size)
{
    return L4_address(page) <= (unsigned)object_base &&
           L4_address(page) + L4_size(page) >
               (unsigned)object_base + object_size;
}

static void syscall_thread_control_delete(unsigned *sp, struct tcb_t *dest_tcb)
{
    if (dest_tcb == get_root_tcb())
    {
        panic("The root thread exited!");
    }
    delete_thread(dest_tcb);
    sp[THREAD_CTX_STACK_R0] = 1;
    // Only unblock caller if they haven't deleted themself right now
    if (dest_tcb != caller)
    {
        unblock_caller();
    }
}

static void syscall_thread_control_modify(unsigned *sp, struct tcb_t *dest_tcb,
                                          L4_thread_id dest,
                                          L4_thread_id space_specifier,
                                          L4_thread_id scheduler,
                                          L4_thread_id pager,
                                          void *utcb_location)
{
    struct tcb_t *space_specifier_tcb =
        find_thread_by_global_id(space_specifier);
    struct tcb_t *scheduler_tcb = NULL;
    if (!space_specifier_tcb)
    {
        sp[THREAD_CTX_STACK_R0] = 0;
        caller->utcb->error = L4_error_invalid_space;
        return;
    }

    if (!L4_is_nil_thread(scheduler))
    {
        scheduler_tcb = find_thread_by_global_id(scheduler);
        if (scheduler_tcb == NULL)
        {
            sp[THREAD_CTX_STACK_R0] = 0;
            caller->utcb->error = L4_error_invalid_scheduler;
            return;
        }
    }

    // Make sure to check whether the UTCB location is valid in the potentially
    // new address space, not in the current one
    L4_fpage_t utcb_fpage = space_specifier_tcb->as->utcb_page;
    if (!fpage_contains_object(utcb_fpage, utcb_location, 160))
    {
        sp[THREAD_CTX_STACK_R0] = 0;
        caller->utcb->error = L4_error_utcb_area;
        return;
    }

    /* NOTE: We do not check for the existence of the pager thread, because
     * according the L4 spec, specifiying an invalid pager thread should not
     * cause an error (at least there is none given). I guess this is fine
     * as the thread will never start anyway without a pager thread that can
     * send it the start message.
     */

    // All checks have passed, modify the tcb
    dest_tcb->global_id = dest;
    dest_tcb->as = space_specifier_tcb->as;
    if (scheduler_tcb)
        dest_tcb->scheduler = scheduler;
    if (!L4_same_threads(pager, L4_NILTHREAD))
        dest_tcb->pager = pager;
    if ((unsigned)utcb_location != (unsigned)-1)
        dest_tcb->utcb = utcb_location;
    write_utcb(dest_tcb);
    sp[THREAD_CTX_STACK_R0] = 1;
}

static void syscall_thread_control_create(unsigned *sp, L4_thread_id dest,
                                          L4_thread_id space_control,
                                          L4_thread_id scheduler,
                                          L4_thread_id pager,
                                          void *utcb_location)
{
    // WARN: There are definitely bugs here. Like creating a thread in a new
    // address space will not work. Debug this when it's actually used
    // somewhere.
    struct tcb_t *space_control_tcb = find_thread_by_global_id(space_control);
    if (space_control_tcb == NULL)
    {
        sp[THREAD_CTX_STACK_R0] = 0;
        caller->utcb->error = L4_error_invalid_space;
        return;
    }
    if (space_control_tcb->as == NULL)
    {
        sp[THREAD_CTX_STACK_R0] = 0;
        caller->utcb->error = L4_error_invalid_space;
        return;
    }
    struct tcb_t *scheduler_tcb = NULL;
    if (!L4_is_nil_thread(scheduler))
    {
        scheduler_tcb = find_thread_by_global_id(scheduler);
        if (!scheduler_tcb)
        {
            sp[THREAD_CTX_STACK_R0] = 0;
            caller->utcb->error = L4_error_invalid_scheduler;
            return;
        }
    }
    if (!fpage_contains_object(space_control_tcb->as->utcb_page, utcb_location,
                               160))
    {
        sp[THREAD_CTX_STACK_R0] = 0;
        caller->utcb->error = L4_error_kip_area;
        return;
    }

    struct tcb_t *tcb = create_thread(dest);
    tcb->global_id = dest;
    tcb->local_id = (L4_thread_id)utcb_location;
    tcb->utcb = utcb_location;
    tcb->state = L4_is_nil_thread(pager) ? TS_INACTIVE : TS_ACTIVE;
    tcb->as = space_control_tcb->as;
    tcb->next_sibling = space_control_tcb->next_sibling;
    tcb->prev_sibling = space_control_tcb;
    space_control_tcb->next_sibling = tcb;
    tcb->pager = pager;
    tcb->scheduler = scheduler;

    sp[THREAD_CTX_STACK_R0] = 1;
}

void syscall_thread_control()
{
    unsigned *const sp = (unsigned *)caller->ctx.sp;
    const L4_thread_id dest = sp[THREAD_CTX_STACK_R0];
    const L4_thread_id space_specifier = sp[THREAD_CTX_STACK_R1];
    const L4_thread_id scheduler = sp[THREAD_CTX_STACK_R2];
    const L4_thread_id pager = sp[THREAD_CTX_STACK_R3];
    void *utcb_location = (void *)caller->ctx.r[THREAD_CTX_R4];

    if (!L4_is_nil_thread(dest) && L4_thread_no(dest) < L4_USER_THREAD_START)
    {
        ((unsigned *)caller->ctx.sp)[THREAD_CTX_STACK_R0] = 0;
        caller->utcb->error = L4_error_no_privilege;
        return;
    }

    struct tcb_t *const dest_tcb = find_thread_by_global_id(dest);
    if (dest_tcb)
    {
        if (L4_same_threads(space_specifier, L4_NILTHREAD))
        {
            // dest exists, space_specifier == nilthread => Delete dest
            syscall_thread_control_delete(sp, dest_tcb);
        }
        else
        {
            // dest exists, space_specifier != nilthread => Modify dest
            syscall_thread_control_modify(sp, dest_tcb, dest, space_specifier,
                                          scheduler, pager, utcb_location);
            unblock_caller();
        }
    }
    else
    {
        if (L4_same_threads(space_specifier, L4_NILTHREAD))
        {
            sp[THREAD_CTX_STACK_R0] = 0;
            caller->utcb->error = L4_error_invalid_space;
            unblock_caller();
        }
        else
        {
            syscall_thread_control_create(sp, dest, space_specifier, scheduler,
                                          pager, utcb_location);
            unblock_caller();
        }
    }
}
