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

void gpio_set_function(unsigned gpio, enum gpio_function fn, unsigned pad_conf)
{
    mmio_write32((uintptr_t)&PADS[gpio], pad_conf);
    mmio_write32((uintptr_t)&GPIO[gpio].ctrl, fn << GPIO_CTRL_FUNCSEL_LOW);
}
