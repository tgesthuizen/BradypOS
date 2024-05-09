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

static int softirq_pending[SIRQ_LAST];
static void (*const softirq_func[SIRQ_LAST])() = {softirq_svc};

void softirq_schedule(enum softirq_type_t type)
{
    softirq_pending[type] = 1;
    set_thread_state(get_kernel_tcb(), TS_RUNNABLE);
}

void softirq_execute()
{
    int should_retry = 0;
retry:
    for (int i = 0; i < SIRQ_LAST; ++i)
    {
        if (softirq_pending[i])
            softirq_func[i]();
    }

    disable_interrupts();
    for (int i = 0; i < SIRQ_LAST; ++i)
        if (softirq_pending[i])
        {
            should_retry = 1;
            break;
        }
    if (!should_retry)
        set_thread_state(get_kernel_tcb(), TS_INACTIVE);

    enable_interrupts();

    if (should_retry)
        goto retry;
}
