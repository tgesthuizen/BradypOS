#include <kern/thread.h>

extern struct tcb_t *caller;

void syscall_schedule()
{
    unsigned *const sp = caller->ctx.sp;
    const L4_thread_id dest = sp[THREAD_CTX_STACK_R0];
    const unsigned time_control = sp[THREAD_CTX_STACK_R1];
    const unsigned processor_control = sp[THREAD_CTX_STACK_R2];
    const unsigned prio = sp[THREAD_CTX_STACK_R3];
    const unsigned preemption_control = caller->ctx.r[THREAD_CTX_R4];
}
