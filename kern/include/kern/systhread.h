#ifndef BRADYPOS_KERN_SYSTHREAD_H
#define BRADYPOS_KERN_SYSTHREAD_H

#include <kern/thread.h>

void create_sys_threads();
void switch_to_kernel();
void set_kernel_state(enum thread_state_t state);

#endif
