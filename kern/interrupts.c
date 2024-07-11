#include "kern/hardware.h"
#include <kern/interrupts.h>

#define NVIC_ISER 0xE000E100
#define NVIC_ICER 0xE000E180
#define NVIC_ISPR 0xE000E200
#define NVIC_ICPR 0xE000E280
#define NVIC_IPRn 0xE000E400

static void nvic_set_priority(unsigned interrupt, unsigned prio)
{
    const unsigned register_idx = interrupt / 4;
    const unsigned register_slot = (interrupt & 3) << 4;
    const unsigned register_mask = ~(((1 << 9) - 1) << register_slot);
    volatile unsigned *const target_register =
        &((volatile unsigned *)NVIC_IPRn)[register_idx];
    unsigned value = *target_register;
    value &= register_mask;
    value |= prio << register_slot;
    *target_register = value;
}

enum
{
    SVCALL_PRIORITY = 1,
    PENDSV_PRIORITY = 3,
    SYSTICK_PRIORITY = 2,
    DEFAULT_IRQ_PRIORITY = 2,
};

void interrupts_init()
{
    // BUG: Setting NVIC priorities makes the system hardfault when interrupts
    // are enabled.
    for (int i = 0; i < 32; ++i)
        nvic_set_priority(i, DEFAULT_IRQ_PRIORITY << 30);
    // Disable all IRQs
    *(volatile unsigned *)NVIC_ICER = ~0;
    *(volatile unsigned *)(PPB_BASE + SHPR2_OFFSET) = (SVCALL_PRIORITY << 30);
    *(volatile unsigned *)(PPB_BASE + SHPR3_OFFSET) =
        (PENDSV_PRIORITY << 22) | (SYSTICK_PRIORITY << 30);
}

bool nvis_is_enabled(unsigned interrupt)
{
    return (*(volatile unsigned *)NVIC_ISER >> interrupt) & 1;
}
void nvic_set_enabled(unsigned interrupt, bool enabled)
{
    volatile unsigned *target =
        (volatile unsigned *)(enabled ? NVIC_ISER : NVIC_ICER);
    *target = 1 << interrupt;
}
bool nvic_is_pending(unsigned interrupt)
{
    return (*(volatile unsigned *)NVIC_ISPR >> interrupt) & 1;
}
void nvic_set_pending(unsigned interrupt, bool pends)
{
    volatile unsigned *target =
        (volatile unsigned *)(pends ? NVIC_ISPR : NVIC_ICPR);
    *target = 1 << interrupt;
}
