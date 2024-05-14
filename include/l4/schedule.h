#ifndef BRADYPOS_KERN_SCHEDULE_H
#define BRADYPOS_KERN_SCHEDULE_H

#include <l4/syscalls.h>
#include <l4/thread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint64_t raw;
} L4_clock_t;

inline L4_clock_t L4_clock_add(L4_clock_t l, L4_clock_t r)
{
    return (L4_clock_t){.raw = l.raw + r.raw};
}
inline L4_clock_t L4_clock_add_usec(L4_clock_t l, uint64_t r)
{
    return (L4_clock_t){.raw = l.raw + r};
}
inline L4_clock_t L4_clock_sub(L4_clock_t l, L4_clock_t r)
{
    return (L4_clock_t){.raw = l.raw - r.raw};
}
inline L4_clock_t L4_clock_sub_usec(L4_clock_t l, uint64_t r)
{
    return (L4_clock_t){.raw = l.raw - r};
}

inline bool L4_is_clock_earlier(L4_clock_t l, L4_clock_t r)
{
    return l.raw < r.raw;
}
inline bool L4_is_clock_later(L4_clock_t l, L4_clock_t r)
{
    return l.raw > r.raw;
}
inline bool L4_is_clock_equal(L4_clock_t l, L4_clock_t r)
{
    return l.raw == r.raw;
}
inline bool L4_is_clock_not_equal(L4_clock_t l, L4_clock_t r)
{
    return l.raw != r.raw;
}

inline L4_clock_t L4_system_clock()
{
    register L4_clock_t clock asm("r0");
    asm volatile("movs r7, %[SYS_CLOCK]\n\t"
                 "svc #0\n\t"
                 : "=r"(clock)
                 : [SYS_CLOCK] "i"(SYS_SYSTEM_CLOCK));
    return clock;
}

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

#define L4_never ((L4_time_t){.raw = 0});
#define L4_zero_time ((L4_time_t){.point = 0, .e = 1, .m = 0})

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

inline void L4_thread_switch(L4_thread_id dest)
{
    register L4_thread_id rdest asm("r0") = dest;
    asm volatile("movs r7, %1\n\t"
                 "svc #0\n\t" ::"r"(rdest),
                 "i"(SYS_THREAD_SWITCH)
                 : "r7");
}

inline void L4_yield() { L4_thread_switch(L4_NILTHREAD); }

#endif
