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

static int softirq_pending;
static void (*const softirq_func[SIRQ_LAST])() = {softirq_svc};

void softirq_schedule(enum softirq_type_t type)
{
    softirq_pending |= 1 << type;
    set_thread_state(get_kernel_tcb(), TS_RUNNABLE);
}

void softirq_execute()
{
    int softirq_run;
retry:
    softirq_run = 0;
    for (int i = 0; i < SIRQ_LAST; ++i)
    {
        if (softirq_pending & (1 << i))
        {
            softirq_func[i]();
            softirq_run |= 1 << i;
        }
    }

    disable_interrupts();
    softirq_run = softirq_pending &= ~softirq_run;
    if (softirq_pending != 0)
    {
        set_thread_state(get_kernel_tcb(), TS_INACTIVE);
    }

    enable_interrupts();

    if (softirq_run)
        goto retry;
}
