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

static unsigned char idle_stack[128];
static void idle_task()
{
    while (1)
        asm volatile("wfi\n\t");
}

void create_sys_threads()
{
    kern_id = L4_global_id(46, 1);
    idle_id = L4_global_id(47, 1);
    root_id = L4_global_id(64, 1);
    kern_tcb = insert_thread(NULL, kern_id);
    idle_tcb = insert_thread(NULL, idle_id);
    unsigned *idle_sp = (unsigned *)(idle_stack + 128);
    idle_tcb->ctx.ret = 0xFFFFFFFD;
    idle_sp -= 8;
    idle_sp[0] = 0;                   // r0
    idle_sp[1] = 0;                   // r1
    idle_sp[2] = 0;                   // r2
    idle_sp[3] = 0;                   // r3
    idle_sp[4] = 0;                   // r12
    idle_sp[5] = 0;                   // lr
    idle_sp[6] = (unsigned)idle_task; // pc
    idle_sp[7] = (1 << 24);           // xPSR
    idle_tcb->ctx.sp = (unsigned)idle_sp;
    idle_tcb->priority = ~0;
    set_thread_state(idle_tcb, TS_RUNNABLE);
    root_tcb = insert_thread(NULL, root_id);
}

void switch_to_kernel() { panic("kernel switch not yet implemented\n"); }
struct tcb_t *get_kernel_tcb() { return kern_tcb; }
