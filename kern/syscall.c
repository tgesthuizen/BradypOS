#include <kern/debug.h>
#include <kern/interrupts.h>
#include <kern/kalarm.h>
#include <kern/platform.h>
#include <kern/softirq.h>
#include <kern/systhread.h>
#include <kern/thread.h>
#include <l4/kip.h>
#include <l4/syscalls.h>
#include <l4/thread.h>
#include <stddef.h>

struct tcb_t *caller;
static const char *const syscall_names[] = {
    "SYS_KERNEL_INTERFACE",
    "SYS_SPACE_CONTROL",
    "SYS_THREAD_CONTROL",
    "SYS_PROCESSOR_CONTROL",
    "SYS_MEMORY_CONTROL",
    "SYS_IPC",
    "SYS_LIPC",
    "SYS_UNMAP",
    "SYS_EXCHANGE_REGISTERS",
    "SYS_SYSTEM_CLOCK",
    "SYS_THREAD_SWITCH",
    "SYS_SCHEDULE",
};

static __attribute__((used)) void __isr_svcall()
{
    register unsigned syscall_number asm("r7");
    dbg_log(DBG_INTERRUPT, "Executing SVCall\n");
    unsigned *const psp = get_psp();
    struct tcb_t *const current_thread = get_current_thread();
    kassert(current_thread != NULL);
    dbg_log(DBG_SYSCALL, "Thread %x has issued syscall #%x\n",
            get_current_thread(), syscall_number);
    switch (syscall_number)
    {
    case SYS_THREAD_SWITCH:
        request_reschedule(
            find_thread_by_global_id((L4_thread_id)psp[THREAD_CTX_STACK_R0]));
        break;
    case SYS_SYSTEM_CLOCK:
    {
        uint64_t time = get_current_time();
        psp[THREAD_CTX_STACK_R0] = time & ~(unsigned)0;
        psp[THREAD_CTX_STACK_R1] = time >> 32;
    }
    break;
    default:
        caller = current_thread;
        set_thread_state(current_thread, TS_SVC_BLOCKED);
        softirq_schedule(SIRQ_SVC_CALL);
        request_reschedule(get_kernel_tcb());
        break;
    }
}

DECLARE_ISR(isr_svcall, __isr_svcall)

void softirq_svc()
{
    const unsigned syscall_id = caller->ctx.r[THREAD_CTX_R7];
    switch (syscall_id)
    {
    case SYS_KERNEL_INTERFACE:
        extern kip_t the_kip;
        ((unsigned *)caller->ctx.sp)[THREAD_CTX_STACK_R0] = (unsigned)&the_kip;
        ((unsigned *)caller->ctx.sp)[THREAD_CTX_STACK_R1] =
            the_kip.api_version.raw;
        ((unsigned *)caller->ctx.sp)[THREAD_CTX_STACK_R2] =
            the_kip.api_flags.raw;
        ((unsigned *)caller->ctx.sp)[THREAD_CTX_STACK_R3] =
            ((struct kern_desc *)L4_read_kip_ptr(&the_kip,
                                                 the_kip.kern_desc_ptr))
                ->kernel_id.raw;
        break;
    case SYS_SPACE_CONTROL:
        extern void syscall_space_control();
        syscall_space_control();
        break;
    case SYS_THREAD_CONTROL:
        extern void syscall_thread_control();
        syscall_thread_control();
        break;
    case SYS_IPC:
        extern void syscall_ipc();
        syscall_ipc();
        break;
    case SYS_THREAD_SWITCH:
    case SYS_SYSTEM_CLOCK:
        panic(
            "Kernel thread is meant to handle syscall %u (%s), but ISR should "
            "have handled it\n",
            syscall_id, syscall_names[syscall_id]);
        break;
    case SYS_SCHEDULE:
        extern void syscall_schedule();
        syscall_schedule();
        break;
    case SYS_PROCESSOR_CONTROL:
    case SYS_MEMORY_CONTROL:
    case SYS_LIPC:
    case SYS_UNMAP:
    case SYS_EXCHANGE_REGISTERS:

        panic("%s is not yet implemented\n", syscall_names[syscall_id]);
        break;
    }

    disable_interrupts();
    set_thread_state(caller, TS_RUNNABLE);
    enable_interrupts();
}
