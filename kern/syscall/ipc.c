#include <kern/debug.h>
#include <kern/kalarm.h>
#include <kern/memory.h>
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

enum ipc_partner_search_result
{
    ipc_partner_nonexistent,
    ipc_partner_notready,
    ipc_partner_found,
    ipc_partner_pager_msg,
};

static bool is_viable_ipc_partner(struct tcb_t *ipc_partner)
{
    if (ipc_partner->ipc_from == L4_ANYTHREAD)
        return true;
    if (ipc_partner->ipc_from == L4_ANYLOCALTHREAD &&
        ipc_partner->as == caller->as)
        return true;
    return ipc_partner->ipc_from == caller->global_id ||
           ipc_partner->ipc_from == caller->local_id;
}

static enum ipc_partner_search_result
find_ipc_partner_any(bool sending, struct tcb_t **tcb_out)
{
    const unsigned thread_count = get_thread_count();
    const enum thread_state_t target_state =
        sending ? TS_RECV_BLOCKED : TS_SEND_BLOCKED;
    for (unsigned i = 0; i < thread_count; ++i)
    {
        struct tcb_t *const current_tcb = get_thread_by_index(i);
        if (current_tcb->state == target_state &&
            is_viable_ipc_partner(current_tcb))
        {
            *tcb_out = current_tcb;
            return ipc_partner_found;
        }
    }
    return ipc_partner_notready;
}

static enum ipc_partner_search_result
find_ipc_partner_anylocal(bool sending, struct tcb_t **tcb_out)
{
    const enum thread_state_t target_state =
        sending ? TS_RECV_BLOCKED : TS_SEND_BLOCKED;
    for (struct tcb_t *current_tcb = caller->as->first_thread; current_tcb;
         current_tcb = current_tcb->next_sibling)
    {
        if (current_tcb->state == target_state &&
            is_viable_ipc_partner(current_tcb))
        {
            *tcb_out = current_tcb;
            return ipc_partner_found;
        }
    }
    return ipc_partner_notready;
}

static enum ipc_partner_search_result
find_ipc_partner(L4_thread_id who, bool sending, struct tcb_t **tcb_out)
{
    if (who == L4_ANYTHREAD)
    {
        return find_ipc_partner_any(sending, tcb_out);
    }
    if (who == L4_ANYLOCALTHREAD)
    {
        return find_ipc_partner_anylocal(sending, tcb_out);
    }
    struct tcb_t *tcb = find_thread_by_global_id(who);
    const enum thread_state_t target_state =
        sending ? TS_RECV_BLOCKED : TS_SEND_BLOCKED;
    if (tcb == NULL)
        return ipc_partner_nonexistent;
    if (tcb->state == target_state && is_viable_ipc_partner(tcb))
    {
        *tcb_out = tcb;
        return ipc_partner_found;
    }
    struct tcb_t *const pager_tcb = find_thread_by_global_id(tcb->pager);
    if (sending && tcb->state == TS_ACTIVE && pager_tcb != NULL &&
        pager_tcb->as == caller->as)
    {
        *tcb_out = tcb;
        return ipc_partner_pager_msg;
    }
    return ipc_partner_notready;
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
    if (timeout.point)
    {
        panic("IPC does not yet support timepoints as timeout: %#04x\n",
              timeout.raw);
    }

    disable_interrupts();
    thread->kevent.data = thread->global_id;
    thread->kevent.type = KALARM_TIMEOUT;
    thread->kevent.when = get_current_time() + (timeout.m << timeout.e) / 10;
    register_kalarm_event(&thread->kevent);
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

static inline void set_ipc_error()
{
    ((L4_msg_tag_t *)&caller->utcb->mr[0])->flags |=
        1 << L4_msg_tag_flag_error_indicator;
}

enum ipc_phase_result
{
    ipc_phase_done,
    ipc_phase_error,
    ipc_phase_wait,
};

static void do_ipc(struct tcb_t *target);

static enum ipc_phase_result ipc_send(struct tcb_t *target, L4_thread_id to,
                                      L4_time_t timeout)
{
    if (L4_is_nil_thread(to))
    {
        dbg_log(DBG_IPC, "Skipping IPC send phase for thread %#08x\n",
                target->global_id);
        return ipc_phase_done;
    }
    struct tcb_t *to_tcb = NULL;
    switch (find_ipc_partner(to, true, &to_tcb))
    {
    case ipc_partner_found:
    {
        dbg_log(DBG_IPC, "Initiating IPC between thread %#08x and %#08x\n",
                target->global_id, to_tcb->global_id);
        const enum L4_ipc_error_code ret = copy_payload(target, to_tcb);
        if (ret != L4_ipc_error_none)
        {
            target->utcb->error =
                (L4_ipc_error_t){
                    .p = 0, .err = L4_ipc_error_message_overflow, .offset = 0}
                    .raw;
            return ipc_phase_error;
        }
        return ipc_phase_done;
    }
    break;
    case ipc_partner_pager_msg:
        dbg_log(DBG_IPC,
                "Pager thread %#08x sends start message to thread %#08x\n",
                target->global_id, to_tcb->global_id);
        { // Message from pager to active thread
            const L4_msg_tag_t tag = {.raw = target->utcb->mr[0]};
            if (tag.flags == 0 && tag.label == 0 && tag.t == 0 && tag.u == 2)
            {
                pager_start_thread(target, to_tcb);
                return ipc_phase_done;
            }
        }
        break;
    case ipc_partner_nonexistent:
        dbg_log(DBG_IPC,
                "Requested IPC partner for %#08x, %#08x, does not exist\n",
                target->global_id, to);
        {
            target->utcb->error =
                (L4_ipc_error_t){.p = 0,
                                 .err = L4_ipc_error_nonexistent_partner,
                                 .offset = 0}
                    .raw;
            return ipc_phase_error;
        }
        break;
    case ipc_partner_notready:
        dbg_log(DBG_IPC,
                "Requested IPC send partner for %#08x, %#08x, is not ready, "
                "blocking\n",
                target->global_id, to_tcb->global_id);
        if (set_thread_timeout(target, timeout, TS_SEND_BLOCKED))
        {
            target->ipc_from = to;
            return ipc_phase_wait;
        }
        else
        {
            target->utcb->error = (L4_ipc_error_t){.p = 0,
                                                   .err = L4_ipc_error_aborted,
                                                   .offset = 0}
                                      .raw;
            return ipc_phase_error;
        }
        break;
    }
    panic("Invalid case encountered in %s\n", __FUNCTION__);
}

static enum ipc_phase_result ipc_recv(struct tcb_t *target, L4_thread_id from,
                                      L4_time_t timeout)
{
    if (L4_is_nil_thread(from))
    {
        dbg_log(DBG_IPC, "Skipping receive phase for thread %#08x\n",
                target->global_id);
        return ipc_phase_done;
    }
    struct tcb_t *from_tcb = NULL;
    switch (find_ipc_partner(from, false, &from_tcb))
    {
    case ipc_partner_found:
    {
        dbg_log(DBG_IPC, "Initiating IPC between %#08x and %#08x\n",
                target->global_id, from_tcb->global_id);
        const enum L4_ipc_error_code ret = copy_payload(from_tcb, target);
        if (ret != L4_ipc_error_none)
        {
            target->utcb->error =
                (L4_ipc_error_t){
                    .p = 1, .err = L4_ipc_error_message_overflow, .offset = 0}
                    .raw;
            return ipc_phase_error;
        }
        return ipc_phase_done;
    }
    break;
    case ipc_partner_nonexistent:
    {
        dbg_log(DBG_IPC,
                "IPC receiving partner for thread %#08x, %#08x, does not exist",
                target->global_id, from);
        target->utcb->error =
            (L4_ipc_error_t){
                .p = 1, .err = L4_ipc_error_nonexistent_partner, .offset = 0}
                .raw;
        return ipc_phase_error;
    }
    break;
    case ipc_partner_notready:
        dbg_log(
            DBG_IPC,
            "IPC receiving partner for thread %#08x is not ready, blocking\n",
            target->global_id);
        if (set_thread_timeout(target, timeout, TS_RECV_BLOCKED))
        {
            target->ipc_from = from;
            return ipc_phase_wait;
        }
        else
        {
            target->utcb->error = (L4_ipc_error_t){.p = 1,
                                                   .err = L4_ipc_error_aborted,
                                                   .offset = 0}
                                      .raw;
            return ipc_phase_error;
        }
        break;
    case ipc_partner_pager_msg:
        panic("Encountered pager message in %s from thread %#08x to %#08x",
              __FUNCTION__, from, target->global_id);
        break;
    }

    panic("Invalid case encountered in %s\n", __FUNCTION__);
}

static void do_ipc(struct tcb_t *target)
{
    unsigned *const sp = (unsigned *)target->ctx.sp;
    const L4_thread_id to = (L4_thread_id)sp[THREAD_CTX_STACK_R0];
    const L4_thread_id from_specifier = (L4_thread_id)sp[THREAD_CTX_STACK_R1];
    const unsigned timeout = sp[THREAD_CTX_STACK_R2];
    L4_thread_id from = L4_NILTHREAD;

    // Skip sending if we're receive blocked
    enum ipc_phase_result send_result = ipc_phase_done;
    if (target->state != TS_RECV_BLOCKED)
    {
        send_result = ipc_send(target, to, L4_send_timeout(timeout));
    }
    switch (send_result)
    {
    case ipc_phase_done:
        break;
    case ipc_phase_error:
        set_ipc_error();
        set_thread_state(target, TS_RUNNABLE);
        goto done;
    case ipc_phase_wait:
        set_thread_state(target, TS_SEND_BLOCKED);
        return;
    }

    switch (ipc_recv(target, from_specifier, L4_recv_timeout(timeout)))
    {
    case ipc_phase_done:
        set_thread_state(target, TS_RUNNABLE);
        break;
    case ipc_phase_error:
        set_ipc_error();
        set_thread_state(target, TS_RUNNABLE);
        goto done;
    case ipc_phase_wait:
        set_thread_state(target, TS_RECV_BLOCKED);
        return;
    }

done:
    sp[THREAD_CTX_STACK_R0] = from;
}

void syscall_ipc() { do_ipc(caller); }

void do_ipc_timeout(unsigned data)
{
    const L4_thread_id target = data;
    struct tcb_t *const target_tcb = find_thread_by_global_id(target);
    if (target_tcb == NULL)
    {
        dbg_log(
            DBG_IPC,
            "Ignoring timeout event for thread %#08x: Thread does not exist\n",
            target);
        return;
    }
    if (target_tcb->state != TS_RECV_BLOCKED &&
        target_tcb->state != TS_SEND_BLOCKED)
    {
        dbg_log(DBG_IPC,
                "Ingoring timeout event for thread %#08x: Thread is "
                "not in a blocked state\n",
                target);
        return;
    }

    disable_interrupts();
    L4_msg_tag_t tag = {.raw = target_tcb->utcb->mr[0]};
    tag.flags |= 1 << L4_ipc_flag_error_indicator;
    target_tcb->utcb->mr[0] = tag.raw;
    target_tcb->utcb->error =
        (L4_ipc_error_t){.err = L4_ipc_error_timeout,
                         .p = (target_tcb->state == TS_RECV_BLOCKED)}
            .raw;
    set_thread_state(target_tcb, TS_RUNNABLE);
    enable_interrupts();
}
