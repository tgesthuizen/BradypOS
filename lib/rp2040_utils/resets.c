#include <rp2040/bus_fabric.h>
#include <rp2040/resets.h>

#define RESETS_BASE 0x4000c000

enum reset_registers
{
    reset_reset = 0x0,
    reset_wdsel = 0x4,
    reset_done = 0x8,
};

void disable_component(enum reset_components component)
{
    mmio_set32(RESETS_BASE + reset_reset, 1 << component);
}

void enable_component(enum reset_components component)
{
    mmio_clear32(RESETS_BASE + reset_reset, 1 << component);
}

void reset_component(enum reset_components component)
{
    disable_component(component);
    enable_component(component);
}

bool component_ready(enum reset_components component)
{
    return (mmio_read32(RESETS_BASE + reset_done) & (1u << component)) != 0;
}
