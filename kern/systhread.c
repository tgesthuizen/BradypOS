#include <kern/debug.h>
#include <kern/systhread.h>
#include <kern/thread.h>
#include <l4/thread.h>
#include <stddef.h>

static struct tcb_t *kern_tcb;
static struct tcb_t *root_tcb;
static struct tcb_t *idle_tcb;

static L4_thread_t kern_id;
static L4_thread_t idle_id;
static L4_thread_t root_id;

void create_sys_threads()
{
    kern_id = L4_global_id(46, 1);
    idle_id = L4_global_id(47, 1);
    root_id = L4_global_id(64, 1);
    kern_tcb = insert_thread(NULL, kern_id);
    idle_tcb = insert_thread(NULL, idle_id);
    root_tcb = insert_thread(NULL, root_id);
}

static void idle_task()
{
    while (1)
        asm volatile("wfi\n\t");
}

void switch_to_kernel() { panic("kernel switch not yet implemented\n"); }
struct tcb_t *get_kernel_tcb() { return kern_tcb; }
