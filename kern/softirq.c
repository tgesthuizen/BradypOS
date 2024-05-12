#include "kern/thread.h"
#include <kern/platform.h>
#include <kern/softirq.h>
#include <kern/systhread.h>

struct softirq_entry_t
{
    int schedule;
    void (*handler)();
};

extern void softirq_svc();

static volatile int softirq_pending;
static void (*const softirq_func[SIRQ_LAST])() = {softirq_svc};

void softirq_schedule(enum softirq_type_t type)
{
    softirq_pending |= 1 << type;
    set_thread_state(get_kernel_tcb(), TS_RUNNABLE);
}

void softirq_execute()
{
    int softirq_pending_buf;
retry:
    softirq_pending_buf = softirq_pending;
    for (int i = 0; i < SIRQ_LAST; ++i)
    {
        if (softirq_pending_buf & (1 << i))
        {
            softirq_func[i]();
        }
    }

    // Assert whether all softirqs are handled and update accordingly.
    disable_interrupts();
    softirq_pending_buf = softirq_pending &= ~softirq_pending_buf;
    if (softirq_pending_buf == 0)
    {
        set_thread_state(get_kernel_tcb(), TS_INACTIVE);
    }

    enable_interrupts();

    if (softirq_pending_buf)
        goto retry;
}
