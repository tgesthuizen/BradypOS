#ifndef BRADYPOS_KERN_SCHEDULE_H
#define BRADYPOS_KERN_SCHEDULE_H

#include <l4/syscalls.h>
#include <l4/thread.h>
#include <l4/time.h>
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
    register unsigned clock_low asm("r0");
    register unsigned clock_high asm("r1");
    asm volatile("movs r7, %[SYS_CLOCK]\n\t"
                 "svc #0\n\t"
                 : "=r"(clock_low), "=r"(clock_high)
                 : [SYS_CLOCK] "i"(SYS_SYSTEM_CLOCK)
                 : "r7");
    return (L4_clock_t){.raw = clock_low | ((uint64_t)clock_high << 32)};
}

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

enum L4_tstate
{
    L4_tstate_error,
    L4_tstate_dead,
    L4_tstate_inactive,
    L4_tstate_running,
    L4_tstate_pending_send,
    L4_tstate_sending,
    L4_tstate_pending_recv,
    L4_tstate_recving,
};

inline unsigned L4_schedule(L4_thread_id dest, unsigned time_control,
                            unsigned processor_control, unsigned prio,
                            unsigned preemption_control,
                            unsigned *old_time_control)
{
    register L4_thread_id rdest asm("r0") = dest;
    register unsigned rtc asm("r1") = time_control;
    register unsigned pc asm("r2") = processor_control;
    register unsigned rprio asm("r3") = prio;
    register unsigned rprec asm("r4") = preemption_control;
    unsigned result;
    unsigned rold_time_control;
    asm volatile("movs r7, %[SYS_SCHEDULE]\n\t"
                 "svc #0\n\t"
                 : "=l"(result), "=l"(rold_time_control)
                 : [SYS_SCHEDULE] "i"(SYS_SCHEDULE), "0"(rdest), "1"(rtc),
                   "l"(pc), "l"(rprio), "l"(rprec)
                 : "r7");
    *old_time_control = rold_time_control;
    return result;
}

inline unsigned L4_set_priority(L4_thread_id dest, unsigned prio)
{
    unsigned old_control;
    return L4_schedule(dest, -1, -1, prio, -1, &old_control);
}

inline unsigned L4_set_processor_no(L4_thread_id dest, unsigned processor_no)
{
    unsigned old_control;
    return L4_schedule(dest, -1, processor_no, -1, -1, &old_control);
}

inline unsigned L4_time_slice(L4_thread_id dest, L4_time_t *ts, L4_time_t *tq)
{
    unsigned old_control;
    const unsigned ret = L4_schedule(dest, -1, -1, -1, -1, &old_control);
    ts->raw = old_control >> 16;
    tq->raw = old_control & (((unsigned)1 << 17) - 1);
    return ret;
}

inline unsigned L4_set_time_slice(L4_thread_id dest, L4_time_t ts, L4_time_t tq)
{
    unsigned old_control;
    return L4_schedule(dest, ts.raw << 16 | tq.raw, -1, -1, -1, &old_control);
}

inline unsigned L4_set_preemption_delay(L4_thread_id dest,
                                        unsigned sensitive_prio,
                                        unsigned max_delay)
{
    unsigned old_control;
    return L4_schedule(dest, -1, -1, -1, sensitive_prio << 16 | max_delay,
                       &old_control);
}

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
