#include <l4/schedule.h>

L4_clock_t L4_clock_add(L4_clock_t l, L4_clock_t r);
L4_clock_t L4_clock_add_usec(L4_clock_t l, uint64_t r);
L4_clock_t L4_clock_sub(L4_clock_t l, L4_clock_t r);
L4_clock_t L4_clock_sub_usec(L4_clock_t l, uint64_t r);
bool L4_is_clock_earlier(L4_clock_t l, L4_clock_t r);
bool L4_is_clock_later(L4_clock_t l, L4_clock_t r);
bool L4_is_clock_equal(L4_clock_t l, L4_clock_t r);
bool L4_is_clock_not_equal(L4_clock_t l, L4_clock_t r);
L4_clock_t L4_system_clock();

L4_time_t L4_time_add_usec(L4_time_t l, uint32_t r)
{
    return (L4_time_t){.e = l.e, .m = l.m + (r >> l.e)};
}

L4_time_t *L4_time_add_usec_to(L4_time_t *l, uint32_t r)
{
    l->m += r >> l->e;
    return l;
}

L4_time_t L4_time_sub_usec(L4_time_t l, uint32_t r)
{
    return (L4_time_t){.e = l.e, .m = l.m - (r >> l.e)};
}

L4_time_t *L4_time_sub_usec_from(L4_time_t *l, uint32_t r)
{
    l->m -= r >> l->e;
    return l;
}

static inline unsigned max_unsigned(unsigned a, unsigned b)
{
    return (a < b ? b : a);
}

static unsigned adj_fix_value(L4_time_t v, unsigned e)
{
    return ((v.m << 22) >> (e - v.e));
}

bool L4_is_time_longer(L4_time_t l, L4_time_t r)
{
    const unsigned exp = max_unsigned(l.e, r.e);
    return adj_fix_value(l, exp) > adj_fix_value(r, exp);
}

bool L4_is_time_shorter(L4_time_t l, L4_time_t r)
{
    const unsigned exp = max_unsigned(l.e, r.e);
    return adj_fix_value(l, exp) < adj_fix_value(r, exp);
}

bool L4_is_time_equal(L4_time_t l, L4_time_t r)
{
    const unsigned exp = max_unsigned(l.e, r.e);
    return adj_fix_value(l, exp) == adj_fix_value(r, exp);
}

bool L4_is_time_not_equal(L4_time_t l, L4_time_t r)
{
    const unsigned exp = max_unsigned(l.e, r.e);
    return adj_fix_value(l, exp) != adj_fix_value(r, exp);
}

unsigned L4_schedule(L4_thread_id dest, unsigned time_control,
                     unsigned processor_control, unsigned prio,
                     unsigned preemption_control, unsigned *old_time_control);
unsigned L4_set_priority(L4_thread_id dest, unsigned prio);
unsigned L4_set_processor_no(L4_thread_id dest, unsigned processor_no);
unsigned L4_time_slice(L4_thread_id dest, L4_time_t *ts, L4_time_t *tq);
unsigned L4_set_time_slice(L4_thread_id dest, L4_time_t ts, L4_time_t tq);
unsigned L4_set_preemption_delay(L4_thread_id dest, unsigned sensitive_prio,
                                 unsigned max_delay);
void L4_thread_switch(L4_thread_id dest);

void L4_thread_switch(L4_thread_id dest);
void L4_yield();
