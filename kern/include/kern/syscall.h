#ifndef BRADYPOS_KERN_SYSCALL_H
#define BRADYPOS_KERN_SYSCALL_H

#include <kern/platform.h>
#include <kern/thread.h>

extern struct tcb_t *caller;

inline void unblock_caller()
{
    disable_interrupts();
    set_thread_state(caller, TS_RUNNABLE);
    enable_interrupts();
}

void syscall_thread_control();
void syscall_space_control();
void syscall_ipc();
void syscall_schedule();

#endif
