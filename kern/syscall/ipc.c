#include <kern/debug.h>
#include <kern/platform.h>
#include <kern/syscall.h>
#include <kern/thread.h>
#include <l4/errors.h>
#include <l4/ipc.h>
#include <l4/schedule.h>
#include <l4/thread.h>
#include <l4/utcb.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

static enum L4_ipc_error_code copy_payload(struct tcb_t *from, struct tcb_t *to)
{
    L4_msg_tag_t msg_tag = {.raw = from->utcb->mr[0]};
    unsigned payload_size = msg_tag.u + msg_tag.t + 1;
    if (payload_size > L4_MR_COUNT)
    {
        return L4_ipc_error_message_overflow;
    }
    memcpy(to->utcb->mr, from->utcb->mr, sizeof(unsigned) * payload_size);
    to->utcb->sender = from->global_id;
    return L4_ipc_error_none;
}

static bool set_thread_timeout(struct tcb_t *thread, L4_time_t timeout,
                               enum thread_state_t target_state)
{
    (void)thread;
    if (L4_is_time_equal(timeout, L4_never))
    {
        return true;
    }
    if (L4_is_time_equal(timeout, L4_zero_time))
    {
        return false;
    }

    disable_interrupts();
    set_thread_state(thread, target_state);
    enable_interrupts();

    return true;
}

static void pager_start_thread(struct tcb_t *pager, struct tcb_t *target)
{
    // Thread start message
    target->ctx.sp =
        pager->utcb->mr[2] - THREAD_CTX_STACK_WORD_COUNT * sizeof(unsigned);
    target->ctx.ret = 0xFFFFFFFD;
    unsigned *const sp = (unsigned *)target->ctx.sp;
    sp[THREAD_CTX_STACK_PC] = pager->utcb->mr[1];
    sp[THREAD_CTX_STACK_PSR] = (1 << 24); // Thumb bit set
    disable_interrupts();
    set_thread_state(target, TS_RUNNABLE);
    enable_interrupts();
}

enum ipc_phase_result
{
    ipc_phase_done,
    ipc_phase_error,
    ipc_phase_wait,
};

static enum ipc_phase_result ipc_send(L4_thread_id to, L4_time_t timeout)
{
    if (L4_is_nil_thread(to))
        return ipc_phase_done;
    struct tcb_t *to_tcb = find_thread_by_global_id(to);
    if (to_tcb == NULL)
    {
        L4_ipc_error_t ret = {
            .p = 0, .err = L4_ipc_error_nonexistent_partner, .offset = 0};
        caller->utcb->error = ret.raw;
        return ipc_phase_error;
    }

    if (to_tcb->state == TS_RECV_BLOCKED)
    {
        enum L4_ipc_error_code ret = copy_payload(caller, to_tcb);
        if (ret != L4_ipc_error_none)
        {
            L4_ipc_error_t code = {
                .p = 0, .err = L4_ipc_error_message_overflow, .offset = 0};
            caller->utcb->error = code.raw;
            return ipc_phase_error;
        }
    }
    // TODO: Allow any thread in the address space of the pager to send the
    // starting message.
    else if (to_tcb->state == TS_ACTIVE &&
             L4_same_threads(to_tcb->pager, caller->global_id))
    {
        // Message from pager to active thread
        L4_msg_tag_t tag = {.raw = caller->utcb->mr[0]};
        if (tag.flags == 0 && tag.label == 0 && tag.t == 0 && tag.u == 2)
        {
            pager_start_thread(caller, to_tcb);
            return ipc_phase_done;
        }
    }
    else if (set_thread_timeout(caller, timeout, TS_SEND_BLOCKED))
    {
        return ipc_phase_done;
    }
    else
    {
        L4_ipc_error_t code = {
            .p = 0, .err = L4_ipc_error_aborted, .offset = 0};
        caller->utcb->error = code.raw;
        return ipc_phase_error;
    }

    panic("Invalid case encountered in %s", __FUNCTION__);
}

static unsigned ipc_recv(L4_thread_id from, L4_time_t timeout)
{
    if (L4_is_nil_thread(from))
        return 0;
    struct tcb_t *from_tcb = find_thread_by_global_id(from);
    if (from_tcb == NULL)
    {
        caller->utcb->error = L4_error_invalid_thread;
        return ipc_phase_error;
    }

    return ipc_phase_done;
}

static inline void set_ipc_error()
{
    ((L4_msg_tag_t *)&caller->utcb->mr[0])->flags |=
        1 << L4_msg_tag_flag_error_indicator;
}

void syscall_ipc()
{
    unsigned *const sp = (unsigned *)caller->ctx.sp;
    const L4_thread_id to = (L4_thread_id)sp[THREAD_CTX_STACK_R0];
    const L4_thread_id from_specifier = (L4_thread_id)sp[THREAD_CTX_STACK_R1];
    const unsigned timeout = sp[THREAD_CTX_STACK_R2];
    L4_thread_id from = L4_NILTHREAD;

    switch (ipc_send(to, L4_send_timeout(timeout)))
    {
    case ipc_phase_done:
        break;
    case ipc_phase_error:
        set_ipc_error();
        set_thread_state(caller, TS_RUNNABLE);
        goto done;
    case ipc_phase_wait:
        set_thread_state(caller, TS_SEND_BLOCKED);
        return;
    }

    switch (ipc_recv(from_specifier, L4_recv_timeout(timeout)))
    {
    case ipc_phase_done:
        break;
    case ipc_phase_error:
        set_ipc_error();
        set_thread_state(caller, TS_RUNNABLE);
        goto done;
    case ipc_phase_wait:
        set_thread_state(caller, TS_RECV_BLOCKED);
        return;
    }

    set_thread_state(caller, TS_RUNNABLE);

done:
    sp[THREAD_CTX_STACK_R0] = from;
}

void ipc_perform_timeout(unsigned data)
{
    const L4_thread_id target = data;
    struct tcb_t *target_tcb = find_thread_by_global_id(target);
    if (target_tcb == NULL)
    {
        dbg_log(DBG_IPC,
                "Ignoring timeout event for thread %u: Thread does not exist\n",
                target);
        return;
    }

    disable_interrupts();
    unsigned *const sp = (unsigned *)target_tcb->ctx.sp;

    enable_interrupts();
}
