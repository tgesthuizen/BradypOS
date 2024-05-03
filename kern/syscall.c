#include <kern/debug.h>
#include <kern/softirq.h>
#include <kern/thread.h>
#include <l4/syscalls.h>

extern struct tcb_t *current_thread;
struct tcb_t *caller;

void isr_svcall()
{
    caller = current_thread;
    set_thread_state(current_thread, TS_SVC_BLOCKED);
    softirq_schedule(SIRQ_SVC_CALL);
    request_reschedule();
}

static __attribute__((noreturn)) void fail_syscall(const char *type)
{
    panic("%s is not yet implemented\n", type);
}

static const char *const syscall_names[] = {
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
    const unsigned syscall_id = caller->ctx.r[7];
    switch (syscall_id)
    {
    case SYS_SPACE_CONTROL:
    case SYS_THREAD_CONTROL:
    case SYS_PROCESSOR_CONTROL:
    case SYS_MEMORY_CONTROL:
    case SYS_IPC:
    case SYS_LIPC:
    case SYS_UNMAP:
    case SYS_EXCHANGE_REGISTERS:
    case SYS_SYSTEM_CLOCK:
    case SYS_THREAD_SWITCH:
    case SYS_SCHEDULE:
        fail_syscall(syscall_names[syscall_id]);
        break;
    }
}
