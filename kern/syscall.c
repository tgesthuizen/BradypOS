#include "kern/kalarm.h"
#include <kern/debug.h>
#include <kern/interrupts.h>
#include <kern/platform.h>
#include <kern/softirq.h>
#include <kern/systhread.h>
#include <kern/thread.h>
#include <l4/kip.h>
#include <l4/syscalls.h>
#include <l4/thread.h>
#include <stddef.h>

struct tcb_t *caller;

static unsigned *get_psp()
{
    unsigned *res;
    asm volatile("mrs %0, PSP\n" : "=r"(res));
    return res;
}

static __attribute__((used)) void __isr_svcall()
{
    register unsigned syscall_number asm("r7");
    unsigned *const psp = get_psp();
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
    {
        struct tcb_t *const current_thread = get_current_thread();
        caller = current_thread;
        set_thread_state(current_thread, TS_SVC_BLOCKED);
        softirq_schedule(SIRQ_SVC_CALL);
        request_reschedule(get_kernel_tcb());
    }
    break;
    }
}

DECLARE_ISR(isr_svcall, __isr_svcall)

static __attribute__((noreturn)) void fail_syscall(const char *type)
{
    panic("%s is not yet implemented\n", type);
}

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
    case SYS_THREAD_SWITCH:
    case SYS_SYSTEM_CLOCK:
        panic(
            "Kernel thread is meant to handle syscall %u (%s), but ISR should "
            "have handled it\n",
            syscall_id, syscall_names[syscall_id]);
        break;
    case SYS_SPACE_CONTROL:
    case SYS_THREAD_CONTROL:
    case SYS_PROCESSOR_CONTROL:
    case SYS_MEMORY_CONTROL:
    case SYS_IPC:
    case SYS_LIPC:
    case SYS_UNMAP:
    case SYS_EXCHANGE_REGISTERS:
    case SYS_SCHEDULE:
        fail_syscall(syscall_names[syscall_id]);
        break;
    }

    disable_interrupts();
    set_thread_state(caller, TS_RUNNABLE);
    enable_interrupts();
}
