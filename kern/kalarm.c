#include "kern/interrupts.h"
#include <kern/debug.h>
#include <kern/kalarm.h>
#include <kern/systick.h>
#include <stdbool.h>

struct kalarm_event
{
    unsigned long when;
    void (*what)();
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

void register_kalarm_event(unsigned long when, void (*what)())
{
    if (kalarm_heap_insert((struct kalarm_event){.when = when, .what = what}) ==
        kalarm_heap_fail)
    {
        panic("Kernel cannot queue kalarm event\n");
    }
}

static bool has_pending_kalarm()
{
    return kalarm_heap_state.size > 0 &&
           kalarm_heap_state.data[0].when <= current_time;
}

static struct kalarm_event pop_kalarm_event()
{
    kassert(kalarm_count);
    const struct kalarm_event result = kalarm_heap_state.data[0];
    kalarm_heap_pop();
    return result;
}

static __attribute__((used)) void __isr_systick()
{
    ++current_time;
    while (has_pending_kalarm())
    {
        pop_kalarm_event().what();
    }
}
DECLARE_ISR(isr_systick, __isr_systick)
     
unsigned long get_current_time() { return current_time; }
