#include "kern/hardware.h"
#include <kern/platform.h>
#include <kern/thread.h>

struct tcb_t *current_task;

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

void request_reschedule()
{
    *(volatile unsigned *)(PPB_BASE + ICSR_OFFSET) |= ICSR_PENDSVSET;
}
