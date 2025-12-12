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

void gpio_set_function(unsigned gpio, enum gpio_function fn);

#endif
