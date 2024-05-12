#include <kern/debug.h>
#include <kern/softirq.h>
#include <kern/systhread.h>
#include <kern/thread.h>
#include <l4/bootinfo.h>
#include <l4/schedule.h>
#include <l4/thread.h>
#include <stddef.h>
#include <string.h>

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

static unsigned char kern_stack[256];
static void kern_task()
{
    while (1)
    {
        softirq_execute();
        L4_yield();
    }
}

void *root_utcb;
void *root_stack;
void *root_got;

void create_sys_threads()
{
    kern_id = L4_global_id(46, 1);
    kern_tcb = insert_thread(NULL, kern_id);
    unsigned *kern_sp = (unsigned *)(kern_stack + 256);
    kern_sp -= 8;
    kern_sp[0] = 0;                   // r0
    kern_sp[1] = 0;                   // r1
    kern_sp[2] = 0;                   // r2
    kern_sp[3] = 0;                   // r3
    kern_sp[4] = 0;                   // r12
    kern_sp[5] = 0;                   // lr
    kern_sp[6] = (unsigned)kern_task; // pc
    kern_sp[7] = (1 << 24);           // xPSR
    kern_tcb->ctx.sp = (unsigned)kern_sp;
    kern_tcb->ctx.ret = 0xFFFFFFFD;
    kern_tcb->priority = 0;
    set_thread_state(kern_tcb, TS_INACTIVE);

    idle_id = L4_global_id(47, 1);
    idle_tcb = insert_thread(NULL, idle_id);
    unsigned *idle_sp = (unsigned *)(idle_stack + 128);
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
    idle_tcb->ctx.ret = 0xFFFFFFFD;
    idle_tcb->priority = ~0;
    set_thread_state(idle_tcb, TS_RUNNABLE);

    extern struct L4_boot_info_header __L4_boot_info;
    struct L4_boot_info_record_exe *current_record =
        (struct L4_boot_info_record_exe *)((unsigned char *)&__L4_boot_info +
                                           __L4_boot_info.first_entry);
    static const unsigned char root_name[] = "root";
    while (current_record->type != L4_BOOT_INFO_EXE ||
           memcmp(&current_record->label, root_name, 4) != 0)
    {
        current_record =
            (struct L4_boot_info_record_exe *)(((unsigned char *)
                                                    current_record) +
                                               current_record->offset_next);
    }
    root_id = L4_global_id(64, 1);
    root_tcb = insert_thread(NULL, root_id);
    unsigned *root_sp = (unsigned *)root_stack;
    root_sp -= 8;
    root_sp[0] = 0;                                    // r0
    root_sp[1] = 0;                                    // r1
    root_sp[2] = 0;                                    // r2
    root_sp[3] = 0;                                    // r3
    root_sp[4] = 0;                                    // r12
    root_sp[5] = 0;                                    // lr
    root_sp[6] = (unsigned)current_record->initial_ip; // pc
    root_sp[7] = (1 << 24);                            // xPSR
    root_tcb->ctx.sp = (unsigned)root_sp;
    root_tcb->ctx.ret = 0xFFFFFFFD;
    root_tcb->priority = 42;
    set_thread_state(root_tcb, TS_RUNNABLE);
}

void switch_to_kernel() { panic("kernel switch not yet implemented\n"); }
struct tcb_t *get_kernel_tcb() { return kern_tcb; }
