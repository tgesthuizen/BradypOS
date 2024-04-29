#ifndef BRADYPOS_KERN_NVIC_H
#define BRADYPOS_KERN_NVIC_H

#include <stdbool.h>

enum exception_nums
{
    EXC_RESET = 1,
    EXC_NMI = 2,
    EXC_HARD_FAULT = 3,
    EXC_SVCALL = 11,
    EXC_PENDSV = 14,
    EXC_SYSTICK = 15,
    EXC_EXTERNAL_BASE = 16,
};

static inline unsigned irq_to_exception_number(unsigned irq_num)
{
    return EXC_EXTERNAL_BASE + irq_num;
}

void nvic_init();
bool nvic_is_enabled(unsigned interrupt);
void nvic_set_enabled(unsigned interrupt, bool enabled);
bool nvic_is_pending(unsigned interrupt);
void nvic_set_pending(unsigned interrupt, bool pends);

#endif
