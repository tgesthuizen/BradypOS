#ifndef BRADYPOS_KERN_TASK_H
#define BRADYPOS_KERN_TASK_H

#include <l4/thread.h>
#include <l4/utcb.h>

enum thread_number_bounds
{
    SYSTEM_THREAD_START = 0x30,
    USER_THREAD_START = 0x40,
    THREAD_COUNT = (1 << 16),
};

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

enum thread_context_registers
{
    THREAD_CTX_R8,
    THREAD_CTX_R9,
    THREAD_CTX_R10,
    THREAD_CTX_R11,
    THREAD_CTX_R4,
    THREAD_CTX_R5,
    THREAD_CTX_R6,
    THREAD_CTX_R7,
};

enum thread_context_stack_registers
{
    THREAD_CTX_STACK_R0,
    THREAD_CTX_STACK_R1,
    THREAD_CTX_STACK_R2,
    THREAD_CTX_STACK_R3,
    THREAD_CTX_STACK_R12,
    THREAD_CTX_STACK_LR,
    THREAD_CTX_STACK_PC,
    THREAD_CTX_STACK_PSR,
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
    L4_thread_id global_id;
    L4_thread_id local_id;
    enum thread_state_t state;
    struct thread_context_t ctx;
    struct as_t *as;
    L4_utcb_t *utcb;
    unsigned priority;

    L4_thread_id ipc_from;
    unsigned timeout_event;
};

void init_thread_system();
struct tcb_t *find_thread_by_global_id(L4_thread_id id);
struct tcb_t *insert_thread(L4_utcb_t *utcb, L4_thread_id id);
void request_reschedule(struct tcb_t *target);
void schedule_next_thread();
void set_thread_state(struct tcb_t *thread, enum thread_state_t state);
struct tcb_t *thread_tcb(L4_thread_id thread);
struct tcb_t *get_current_thread();
__attribute__((noreturn)) void start_scheduling();

#endif
