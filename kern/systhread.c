#include <kern/debug.h>
#include <kern/interrupts.h>
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

static L4_thread_id kern_id;
static L4_thread_id idle_id;
static L4_thread_id root_id;

enum
{
    IDLE_STACK_SIZE = 128,
    KERN_STACK_SIZE = 512,
};

static unsigned char idle_stack[IDLE_STACK_SIZE];
static void idle_task()
{
    while (1)
        asm volatile("wfi\n\t");
}

static unsigned char kern_stack[KERN_STACK_SIZE];
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
    register unsigned kern_got asm("r9");

    kern_id = L4_global_id(46, 1);
    kern_tcb = create_thread(kern_id);
    kern_tcb->scheduler = kern_id;
    kern_tcb->pager = kern_id;
    unsigned *kern_sp = (unsigned *)(kern_stack + KERN_STACK_SIZE);
    kern_sp -= THREAD_CTX_STACK_WORD_COUNT;
    kern_sp[THREAD_CTX_STACK_R0] = 0;
    kern_sp[THREAD_CTX_STACK_R1] = 0;
    kern_sp[THREAD_CTX_STACK_R2] = 0;
    kern_sp[THREAD_CTX_STACK_R3] = 0;
    kern_sp[THREAD_CTX_STACK_R12] = 0;
    kern_sp[THREAD_CTX_STACK_LR] = 0;
    kern_sp[THREAD_CTX_STACK_PC] = (unsigned)kern_task;
    kern_sp[THREAD_CTX_STACK_PSR] = (1 << 24);
    kern_tcb->ctx.r[THREAD_CTX_R9] = kern_got;
    kern_tcb->ctx.sp = (unsigned)kern_sp;
    kern_tcb->ctx.ret = EXCEPTION_RETURN_TO_THREAD_ON_PROCESS_STACK;
    kern_tcb->priority = 0;
    set_thread_state(kern_tcb, TS_INACTIVE);

    idle_id = L4_global_id(47, 1);
    idle_tcb = create_thread(idle_id);
    idle_tcb->pager = kern_id;
    idle_tcb->scheduler = kern_id;
    unsigned *idle_sp = (unsigned *)(idle_stack + IDLE_STACK_SIZE);
    idle_sp -= THREAD_CTX_STACK_WORD_COUNT;
    idle_sp[THREAD_CTX_STACK_R0] = 0;
    idle_sp[THREAD_CTX_STACK_R1] = 0;
    idle_sp[THREAD_CTX_STACK_R2] = 0;
    idle_sp[THREAD_CTX_STACK_R3] = 0;
    idle_sp[THREAD_CTX_STACK_R12] = 0;
    idle_sp[THREAD_CTX_STACK_LR] = 0;
    idle_sp[THREAD_CTX_STACK_PC] = (unsigned)idle_task;
    idle_sp[THREAD_CTX_STACK_PSR] = (1 << 24);
    idle_tcb->ctx.r[THREAD_CTX_R9] = kern_got;
    idle_tcb->ctx.sp = (unsigned)idle_sp;
    idle_tcb->ctx.ret = EXCEPTION_RETURN_TO_THREAD_ON_PROCESS_STACK;
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
    root_tcb = create_thread(root_id);
    root_tcb->scheduler = kern_id;
    root_tcb->pager = kern_id;
    root_tcb->utcb = root_utcb;
    root_tcb->local_id = (L4_thread_id)root_utcb;
    unsigned *root_sp = (unsigned *)root_stack;
    root_sp -= 8;
    root_sp[THREAD_CTX_STACK_R0] = 0;
    root_sp[THREAD_CTX_STACK_R1] = 0;
    root_sp[THREAD_CTX_STACK_R2] = 0;
    root_sp[THREAD_CTX_STACK_R3] = 0;
    root_sp[THREAD_CTX_STACK_R12] = 0;
    root_sp[THREAD_CTX_STACK_LR] = 0;
    root_sp[THREAD_CTX_STACK_PC] = (unsigned)current_record->initial_ip;
    root_sp[THREAD_CTX_STACK_PSR] = (1 << 24);
    root_tcb->ctx.r[THREAD_CTX_R9] = (unsigned)root_got;
    root_tcb->ctx.sp = (unsigned)root_sp;
    root_tcb->ctx.ret = EXCEPTION_RETURN_TO_THREAD_ON_PROCESS_STACK;
    root_tcb->priority = 42;
    set_thread_state(root_tcb, TS_RUNNABLE);
}

struct tcb_t *get_kernel_tcb() { return kern_tcb; }
struct tcb_t *get_root_tcb() { return root_tcb; }
