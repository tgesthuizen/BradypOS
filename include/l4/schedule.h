#ifndef BRADYPOS_KERN_SCHEDULE_H
#define BRADYPOS_KERN_SCHEDULE_H

#include <stdbool.h>
#include <stdint.h>
#include <types.h>

typedef struct
{
    uint64_t raw;
} L4_clock_t;

static inline L4_clock_t L4_clock_add(L4_clock_t l, L4_clock_t r)
{
    return (L4_clock_t){.raw = l.raw + r.raw};
}
static inline L4_clock_t L4_clock_add_usec(L4_clock_t l, uint64_t r)
{
    return (L4_clock_t){.raw = l.raw + r};
}
static inline L4_clock_t L4_clock_sub(L4_clock_t l, L4_clock_t r)
{
    return (L4_clock_t){.raw = l.raw - r.raw};
}
static inline L4_clock_t L4_clock_sub_usec(L4_clock_t l, uint64_t r)
{
    return (L4_clock_t){.raw = l.raw - r};
}

static inline bool L4_is_clock_earlier(L4_clock_t l, L4_clock_t r)
{
    return l.raw < r.raw;
}
static inline bool L4_is_clock_later(L4_clock_t l, L4_clock_t r)
{
    return l.raw > r.raw;
}
static inline bool L4_is_clock_equal(L4_clock_t l, L4_clock_t r)
{
    return l.raw == r.raw;
}
static inline bool L4_is_clock_not_equal(L4_clock_t l, L4_clock_t r)
{
    return l.raw != r.raw;
}

L4_clock_t L4_system_clock();

typedef union
{
    struct
    {
        unsigned m : 10;
        unsigned e : 5;
        unsigned point : 1;
    };
    uint16_t raw;
} L4_time_t;

static inline L4_time_t L4_never = {.raw = 0};
static inline L4_time_t L4_zero_time = {.point = 0, .e = 1, .m = 0};

L4_time_t L4_time_add_usec(L4_time_t l, uint32_t r);
L4_time_t *L4_time_add_usec_to(L4_time_t *l, uint32_t r);
L4_time_t L4_time_sub_usec(L4_time_t l, uint32_t r);
L4_time_t *L4_time_sub_usec_from(L4_time_t *l, uint32_t r);

L4_time_t L4_time_add(L4_time_t l, L4_time_t r);
L4_time_t *L4_time_add_to(L4_time_t *l, L4_time_t r);
L4_time_t L4_time_sub(L4_time_t l, L4_time_t r);
L4_time_t *L4_time_sub_from(L4_time_t *l, L4_time_t r);

bool L4_is_time_longer(L4_time_t l, L4_time_t r);
bool L4_is_time_shorter(L4_time_t l, L4_time_t r);
bool L4_is_time_equal(L4_time_t l, L4_time_t r);
bool L4_is_time_not_equal(L4_time_t l, L4_time_t r);

void L4_thread_switch(L4_thread_t dest);
void L4_yield();
uint32_t L4_schedule(L4_thread_t dest, uint32_t time_control,
                     uint32_t processor_control, uint32_t prio,
                     uint32_t preemption_control, uint32_t *old_time_control);
static inline uint32_t L4_set_priority(L4_thread_t dest, uint32_t prio)
{
    return L4_schedule(dest, -1, -1, prio, -1);
}
static inline uint32_t L4_set_processor_no(L4_thread_t dest,
                                           uint32_t processor_no)
{
    return L4_schedule(dest, -1, processor_no, -1, -1);
}
uint32_t L4_timeslice(L4_thread_t dest, L4_time_t *ts, L4_time_t *tq);
uint32_t L4_set_timeslice(L4_thread_t dest, L4_time_t ts, L4_time_t tq);
uint32_t L4_set_preemption_delay(uint32_t dest, uint32_t sensitive_prio,
                                 uint32_t max_delay)
{
    return L4_schedule(dest, -1, -1, -1, (sensitive_prio << 16) + max_delay);
}

bool L4_enable_preemption_fault_exception();
bool L4_disable_preemption_fault_exception();
bool L4_disable_preemption();
bool L4_enable_preemption();
bool L4_preemption_pending();

#endif
