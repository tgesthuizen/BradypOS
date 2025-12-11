#include <rp2040/bus_fabric.h>
#include <rp2040/gpio.h>

enum
{
    IO_BANK0_BASE = 0x40014000,
    PADS_BANK0_BASE = 0x4001c000,
};

struct gpio_hw
{
    unsigned status;
    unsigned ctrl;
};

#define GPIO ((volatile struct gpio_hw *)IO_BANK0_BASE)
#define PADS ((volatile unsigned *)(PADS_BANK0_BASE + 4))

enum gpio_ctrl_bits
{
    GPIO_CTRL_FUNCSEL_LOW = 0,
    GPIO_CTRL_FUNCSEL_HIGH = 4,
    GPIO_CTRL_OUTOVER_LOW = 8,
    GPIO_CTRL_OUTOVER_HIGH = 9,
    GPIO_CTRL_OEOVER_LOW = 12,
    GPIO_CTRL_OEOVER_HIGH = 13,
    GPIO_CTRL_INOVER_LOW = 16,
    GPIO_CTRL_INOVER_HIGH = 17,
    GPIO_CTRL_IRQOVER_LOW = 28,
    GPIO_CTRL_IRQOVER_HIGH = 29,
};

enum pads_bank0_bits
{
    PADS_BANK0_SLEWFAST = 0,
    PADS_BANK0_SCHMITT = 1,
    PADS_BANK0_PDE = 2,
    PADS_BANK0_PUE = 3,
    PADS_BANK0_DRIVE_LOW = 4,
    PADS_BANK0_DRIVE_HIGH = 5,
    PADS_BANK0_IE = 6,
    PADS_BANK0_OD = 7,
};

void gpio_set_function(unsigned gpio, enum gpio_function fn)
{
    mmio_write_masked32((uintptr_t)&PADS[gpio], (1 << PADS_BANK0_IE),
                        (1 << PADS_BANK0_IE));
    mmio_write32((uintptr_t)&GPIO[gpio].ctrl, fn << GPIO_CTRL_FUNCSEL_LOW);
}
