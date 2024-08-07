#ifndef BRADYPOS_KERN_SYSCALL_H
#define BRADYPOS_KERN_SYSCALL_H

#include <kern/platform.h>
#include <kern/thread.h>
#include <l4/space.h>
#include <stddef.h>

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
void syscall_unmap();
void syscall_schedule();

bool fpage_contains_object(L4_fpage_t page, void *object_base,
                           size_t object_size);

#endif
