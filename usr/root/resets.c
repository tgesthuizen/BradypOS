#include "resets.h"

#define RESETS_BASE 0x4000c000

enum reset_registers
{
    reset_reset = 0x0,
    reset_wdsel = 0x4,
    reset_done = 0x8,
};

void disable_component(enum reset_components component)
{
    volatile unsigned *reg_reset =
        (volatile unsigned *)(RESETS_BASE + reset_reset);
    unsigned mask = *reg_reset;
    mask &= ~(unsigned)component;
    *reg_reset = mask;
}

void enable_component(enum reset_components component)
{
    volatile unsigned *reg_reset =
        (volatile unsigned *)(RESETS_BASE + reset_reset);
    unsigned mask = *reg_reset;
    mask |= (unsigned)component;
    *reg_reset = mask;
}

void reset_component(enum reset_components component)
{
    disable_component(component);
    enable_component(component);
}

bool component_ready(enum reset_components component)
{
    return ((*(volatile unsigned *)(RESETS_BASE + reset_done)) &
            (unsigned)component) != 0;
}
