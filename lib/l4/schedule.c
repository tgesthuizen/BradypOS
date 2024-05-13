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

void L4_thread_switch(L4_thread_t dest);
void L4_yield();
