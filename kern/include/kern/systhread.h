#ifndef BRADYPOS_KERN_SYSTHREAD_H
#define BRADYPOS_KERN_SYSTHREAD_H

#include <kern/thread.h>

void create_sys_threads();
struct tcb_t *get_kernel_tcb();

#endif
