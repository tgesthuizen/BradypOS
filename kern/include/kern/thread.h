#ifndef BRADYPOS_KERN_TASK_H
#define BRADYPOS_KERN_TASK_H

#include "types.h"
#include <l4/utcb.h>

enum thread_kind_t
{
    TK_KERNEL,
    TK_IDLE,
    TK_ROOT,
    TK_INTERRUPT,
    TK_USER,
};

enum thread_state_t
{
    TS_INACTIVE,
    TS_RUNNABLE,
    TS_SVC_BLOCKED,
    TS_RECV_BLOCKED,
    TS_SEND_BLOCKED,
};

struct thread_context_t
{
    unsigned sp;
    unsigned ret;
    unsigned r[8];
};

struct as_t;

struct tcb_t
{
    L4_thread_t global_id;
    L4_thread_t local_id;
    enum thread_state_t state;
    struct thread_context_t ctx;
    struct as_t *as;
    struct utcb_t *utcb;
    unsigned priority;

    L4_thread_t ipc_from;
    uint32_t timeout_event;
};

void init_thread_system();
struct tcb_t *insert_thread(struct utcb_t *utcb, L4_thread_t id);
void request_reschedule();
void schedule_next_thread();
void set_thread_state(struct tcb_t *thread, enum thread_state_t state);
struct tcb_t *thread_tcb(L4_thread_t thread);
struct tcb_t *get_current_thread();
__attribute__((noreturn)) void start_scheduling();

#endif
