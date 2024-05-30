#include <kern/debug.h>
#include <kern/interrupts.h>
#include <kern/kalarm.h>
#include <kern/platform.h>
#include <kern/thread.h>
#include <stdbool.h>
#include <stddef.h>

static struct kalarm_event *next_event;
static unsigned long current_time;

void kalarm_init()
{
    current_time = 0;
    next_event = NULL;
}

static struct kalarm_event **find_insertion_point(unsigned long when)
{
    struct kalarm_event **cur_ptr = &next_event;
    while (*cur_ptr != NULL && (*cur_ptr)->when < when)
        cur_ptr = &(*cur_ptr)->next;
    return cur_ptr;
}

void register_kalarm_event(struct kalarm_event *ev)
{
    struct kalarm_event **ins_ptr = find_insertion_point(ev->when);
    struct kalarm_event *old_ev = *ins_ptr;
    *ins_ptr = ev;
    ev->prev = old_ev;
    if (old_ev)
    {
        if (old_ev->next != NULL)
            old_ev->next->prev = ev;
        old_ev->next = ev;
    }
}

static void unlink_event(struct kalarm_event *ev)
{
    if (ev->next)
        ev->next->prev = ev->prev;
    if (ev->prev)
        ev->prev->next = ev->next;
    if (next_event == ev)
        next_event = ev->next;
}

void update_kalarm_event(struct kalarm_event *event, unsigned long when)
{
    struct kalarm_event **ins_ptr = find_insertion_point(when);
    unlink_event(event);
    event->when = when;
    event->prev = *ins_ptr;
    event->next = *ins_ptr != NULL ? (*ins_ptr)->next : NULL;
    if (*ins_ptr != NULL)
        (*ins_ptr)->prev = event;
    *ins_ptr = event;
}

static inline bool has_pending_kalarm()
{
    return next_event != NULL && next_event->when <= current_time;
}

static inline struct kalarm_event *pop_kalarm_event()
{
    struct kalarm_event *const next = next_event->next;
    if (next)
    {
        next->prev = NULL;
    }
    struct kalarm_event *const current = next_event;
    next_event = next;
    return current;
}

static void unpause_thread(unsigned data)
{
    struct tcb_t *target = find_thread_by_global_id((L4_thread_id)data);
    if (!target)
    {
        panic("Processing unpause event for non-existent thread %u\n", target);
    }
    disable_interrupts();
    set_thread_state(target, TS_RUNNABLE);
    enable_interrupts();
}
static void request_next_thread(unsigned data)
{
    (void)data;
    request_reschedule(NULL);
}
static void (*kalarm_event_funcs[])(unsigned data) = {request_next_thread,
                                                      unpause_thread};

static __attribute__((used)) void __isr_systick()
{
    dbg_log(DBG_INTERRUPT, "Executing systick\n");
    ++current_time;
    while (has_pending_kalarm())
    {
        struct kalarm_event *const ev = pop_kalarm_event();
        kalarm_event_funcs[ev->type](ev->data);
    }
}
DECLARE_ISR(isr_systick, __isr_systick)

unsigned long get_current_time() { return current_time; }
