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
static struct kalarm_event kalarm_events[MAX_KALARM_EVENTS];
static unsigned kalarm_count;
static unsigned long current_time;

void kalarm_init()
{
    kalarm_count = 0;
    current_time = 0;
}

static void swap_kalarm_event(struct kalarm_event *lhs,
                              struct kalarm_event *rhs)
{
    const struct kalarm_event tmp = *lhs;
    *lhs = *rhs;
    *rhs = tmp;
}

void register_kalarm_event(unsigned long when, void (*what)())
{
    if (kalarm_count == MAX_KALARM_EVENTS)
    {
        panic("Kernel is out of kalarms!\n");
    }
    // Add event
    unsigned idx = kalarm_count++;
    kalarm_events[idx] = (struct kalarm_event){.when = when, .what = what};
    // And heapify
    for (unsigned parent = idx / 2;
         idx && (kalarm_events[idx].when < kalarm_events[parent].when);
         idx = parent, parent = (idx - 1) / 2)
    {
        swap_kalarm_event(&kalarm_events[idx], &kalarm_events[parent]);
    }
}

static bool has_pending_kalarm()
{
    return kalarm_count && kalarm_events[0].when <= current_time;
}
static struct kalarm_event pop_kalarm_event()
{
    const struct kalarm_event result = kalarm_events[0];
    unsigned idx = 0;
    const unsigned leaf_node_idx = kalarm_count / 2;
    while (idx < leaf_node_idx)
    {
        const unsigned idx_left = idx * 2 + 1;
        const unsigned idx_right = idx * 2 + 2;
        const unsigned target_idx =
            (kalarm_events[idx_left].when <= kalarm_events[idx_right].when)
                ? idx_left
                : idx_right;
        kalarm_events[idx] = kalarm_events[target_idx];
        idx = target_idx;
    }
    return result;
}

void isr_systick()
{
    ++current_time;
    while (has_pending_kalarm())
    {
        pop_kalarm_event().what();
    }
}
