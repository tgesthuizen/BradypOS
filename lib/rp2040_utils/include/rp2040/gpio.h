#ifndef BRADYPOS_RP2040_UTILS_GPIO_H
#define BRADYPOS_RP2040_UTILS_GPIO_H

enum gpio_function
{
    GPIO_FN0,
    GPIO_FN1,
    GPIO_FN2,
    GPIO_FN3,
    GPIO_FN4,
    GPIO_FN5,
    GPIO_FN6,
    GPIO_FN7,
    GPIO_FN8,
    GPIO_FN9,
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

enum pad_drive_strength
{
    PAD_DRIVE_STRENGTH_2mA,
    PAD_DRIVE_STRENGTH_4mA,
    PAD_DRIVE_STRENGTH_8mA,
    PAD_DRIVE_STRENGTH_12mA,
};

void gpio_set_function(unsigned gpio, enum gpio_function fn);

#endif
