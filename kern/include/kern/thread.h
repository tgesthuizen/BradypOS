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
    unsigned r[16];
    unsigned ctl;
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

void init_task();
void request_reschedule();

#endif
