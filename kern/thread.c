#include "kern/hardware.h"
#include <kern/platform.h>
#include <kern/thread.h>

enum
{
    THREAD_MAX_COUNT = 16
};

struct tcb_t *current_thread;
static struct tcb_t thread_list[THREAD_MAX_COUNT];
static struct tcb_t *thread_list_sorted[THREAD_MAX_COUNT];
static unsigned thread_count;

enum icsr_bits_t
{
    ICSR_VECT_ACTIVE = (1 << 9) - 1,
    ICSR_VECT_PENDING = ((1 << 9) - 1) << 12,
    ICSR_ISR_PENDING = 1 << 22,
    ICSR_ISR_PREEMPT = 1 << 23,
    ICSR_PENDSTCLR = 1 << 25,
    ICSR_PENDSTSET = 1 << 26,
    ICSR_PENDSVCLR = 1 << 27,
    ICSR_PENDSVSET = 1 << 28,
    ICSR_NMIPENDSET = 1 << 31,
};

void set_thread_state(struct tcb_t *tcb, enum thread_state_t state) {
  tcb->state = state;
  // Update heaps
}

void request_reschedule()
{
    *(volatile unsigned *)(PPB_BASE + ICSR_OFFSET) |= ICSR_PENDSVSET;
}
