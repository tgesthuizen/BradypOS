#include <kern/debug.h>
#include <kern/interrupts.h>
#include <kern/kalarm.h>
#include <kern/systick.h>
#include <kern/thread.h>
#include <stdbool.h>

struct kalarm_event
{
    unsigned long when;
    enum kalarm_type event;
    unsigned data;
};

enum
{
    MAX_KALARM_EVENTS = 16
};

static inline bool kalarm_earlier(struct kalarm_event *lhs,
                                  struct kalarm_event *rhs)
{
    return lhs->when < rhs->when;
}

#define HEAP_VALUE_TYPE struct kalarm_event
#define HEAP_SIZE MAX_KALARM_EVENTS
#define HEAP_COMPARE(a, b) kalarm_earlier(&(a), &(b))
#define HEAP_PREFIX kalarm_heap
#include <heap.inc>
#undef HEAP_VALUE_TYPE
#undef HEAP_SIZE
#undef HEAP_COMPARE
#undef HEAP_PREFIX

static unsigned long current_time;

void kalarm_init()
{
    kalarm_heap_init();
    current_time = 0;
}

static unsigned find_first_kalarm_event(enum kalarm_type type)
{
    for (unsigned i = 0; i < kalarm_heap_state.size; ++i)
        if (kalarm_heap_state.data[i].event == type)
            return i;
    return kalarm_heap_fail;
}

void register_kalarm_event(enum kalarm_type type, unsigned long when)
{
    if (kalarm_heap_insert((struct kalarm_event){
            .when = when, .event = type}) == kalarm_heap_fail)
    {
        panic("Kernel cannot queue kalarm event\n");
    }
}

void update_kalarm_event(enum kalarm_type type, unsigned long when)
{
    unsigned idx = find_first_kalarm_event(type);
    if (idx == kalarm_heap_fail)
        register_kalarm_event(when, type);
    else
    {
        kalarm_heap_state.data[idx].when = when;
        kalarm_heap_sink(kalarm_heap_swim(idx));
    }
}

static bool has_pending_kalarm()
{
    return kalarm_heap_state.size > 0 &&
           kalarm_heap_state.data[0].when <= current_time;
}

static struct kalarm_event pop_kalarm_event()
{
    kassert(kalarm_heap_state.size > 0);
    const struct kalarm_event result = kalarm_heap_state.data[0];
    kalarm_heap_pop();
    return result;
}

static void request_next_thread() { request_reschedule(L4_NILTHREAD); }
static void (*kalarm_event_funcs[])() = {request_next_thread};

static __attribute__((used)) void __isr_systick()
{
    dbg_log(DBG_INTERRUPT, "Executing systick\n");
    ++current_time;
    while (has_pending_kalarm())
    {
        kalarm_event_funcs[pop_kalarm_event().event]();
    }
}
DECLARE_ISR(isr_systick, __isr_systick)

unsigned long get_current_time() { return current_time; }
