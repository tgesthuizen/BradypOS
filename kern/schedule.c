#include "kern/platform.h"
#include <kern/debug.h>
#include <kern/thread.h>
#include <l4/errors.h>
#include <l4/schedule.h>
#include <stddef.h>

extern struct tcb_t *caller;

void syscall_schedule()
{
    unsigned *const sp = (unsigned *)caller->ctx.sp;
    const L4_thread_id dest = sp[THREAD_CTX_STACK_R0];
    const unsigned time_control = sp[THREAD_CTX_STACK_R1];
    const unsigned processor_control = sp[THREAD_CTX_STACK_R2];
    const unsigned prio = sp[THREAD_CTX_STACK_R3];
    const unsigned preemption_control = caller->ctx.r[THREAD_CTX_R4];

    struct tcb_t *const dest_tcb = find_thread_by_global_id(dest);
    if (dest_tcb == NULL)
    {
        caller->utcb->error = L4_error_invalid_thread;
        sp[THREAD_CTX_STACK_R0] = 0;
        return;
    }
    struct tcb_t *dest_scheduler_tcb =
        find_thread_by_global_id(dest_tcb->scheduler);
    if (dest_scheduler_tcb == NULL || caller->as != dest_scheduler_tcb->as)
    {
        caller->utcb->error = L4_error_no_privilege;
        sp[THREAD_CTX_STACK_R0] = 0;
        return;
    }

    (void)time_control;
    if (processor_control != (unsigned)-1 && processor_control != 0)
    {
        caller->utcb->error = L4_error_invalid_parameter;
        sp[THREAD_CTX_STACK_R0] = 0;
        return;
    }
    if (prio != (unsigned)-1 && caller->priority > prio)
    {
        caller->utcb->error = L4_error_invalid_parameter;
        sp[THREAD_CTX_STACK_R0] = 0;
        return;
    }
    if (preemption_control != (unsigned)-1)
    {
        panic("Thread tried to modify preemption control, which is not "
              "implemented\n");
    }

    // All checks passed, apply changes
    disable_interrupts();
    if (prio != (unsigned)-1)
        set_thread_priority(dest_tcb, prio);
    dest_tcb->utcb->processor_no = 0;
    enable_interrupts();
    unsigned tstate = L4_tstate_error;
    switch (dest_tcb->state)
    {
    case TS_INACTIVE:
        tstate = L4_tstate_inactive;
        break;
    case TS_RUNNABLE:
        tstate = L4_tstate_running;
        break;
    case TS_ACTIVE:
    case TS_SVC_BLOCKED:
        // TODO: Is this the right thing to do?
        tstate = L4_tstate_recving;
        break;
    case TS_RECV_BLOCKED:
        tstate = L4_tstate_pending_recv;
        break;
    case TS_SEND_BLOCKED:
        tstate = L4_tstate_pending_send;
        break;
    case TS_PAUSED:
        tstate = L4_tstate_dead;
        break;
    }
    sp[THREAD_CTX_STACK_R0] = tstate;
    // TODO: Revisit when time slices are implemented
    sp[THREAD_CTX_STACK_R1] = 0;
}
