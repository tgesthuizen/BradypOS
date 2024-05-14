#include <kern/debug.h>
#include <kern/interrupts.h>
#include <kern/platform.h>
#include <kern/softirq.h>
#include <kern/systhread.h>
#include <kern/thread.h>
#include <l4/kip.h>
#include <l4/syscalls.h>
#include <l4/thread.h>

struct tcb_t *caller;

static __attribute__((used)) void __isr_svcall()
{
    struct tcb_t *current_thread = get_current_thread();
    if (current_thread != get_kernel_tcb())
    {
        caller = current_thread;
        set_thread_state(current_thread, TS_SVC_BLOCKED);
        softirq_schedule(SIRQ_SVC_CALL);
    }
    request_reschedule();
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
        if (!L4_is_nil_thread(
                (L4_thread_id)((unsigned *)caller->ctx.sp)[THREAD_CTX_STACK_R0]))
            panic("Attempt to switch thread to specific thread %u, not "
                  "implemented yet\n",
                  ((unsigned *)caller->ctx.sp)[THREAD_CTX_STACK_R0]);
        // We don't need to do anything, as we just scheduled from the
        // requesting thread to the kernel.
        break;
    case SYS_SPACE_CONTROL:
    case SYS_THREAD_CONTROL:
    case SYS_PROCESSOR_CONTROL:
    case SYS_MEMORY_CONTROL:
    case SYS_IPC:
    case SYS_LIPC:
    case SYS_UNMAP:
    case SYS_EXCHANGE_REGISTERS:
    case SYS_SYSTEM_CLOCK:
    case SYS_SCHEDULE:
        fail_syscall(syscall_names[syscall_id]);
        break;
    }

    disable_interrupts();
    set_thread_state(caller, TS_RUNNABLE);
    enable_interrupts();
}
