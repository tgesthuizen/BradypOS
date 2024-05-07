#ifndef BRADYPOS_KERN_SCHEDULE_H
#define BRADYPOS_KERN_SCHEDULE_H

#include <l4/syscalls.h>
#include <l4/thread.h>
#include <stdbool.h>
#include <stddef.h>
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

static L4_time_t L4_never = {.raw = 0};
static L4_time_t L4_zero_time = {.point = 0, .e = 1, .m = 0};

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

/* TODO: Implement L4_schedule and friends */

static inline void L4_thread_switch(L4_thread_t dest)
{
    register L4_thread_t rdest asm("r0") = dest;
    asm volatile("movs r7, %1\n\t"
                 "svc #0\n\t" ::"r"(rdest),
                 "i"(SYS_THREAD_SWITCH)
                 : "r7");
}

static inline void L4_yield() { L4_thread_switch(L4_NILTHREAD); }

#endif
