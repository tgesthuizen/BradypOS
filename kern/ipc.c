#include "l4/thread.h"
#include <kern/thread.h>

extern struct tcb_t *caller;

void syscall_ipc()
{
    unsigned *const sp = (unsigned *)caller->ctx.sp;
    const L4_thread_id to = (L4_thread_id)sp[THREAD_CTX_STACK_R0];
    const L4_thread_id from_specifier = (L4_thread_id)sp[THREAD_CTX_STACK_R1];
    const unsigned timeout = sp[THREAD_CTX_STACK_R2];
    L4_thread_id from = L4_NILTHREAD;

    sp[THREAD_CTX_STACK_R0] = from;
}
