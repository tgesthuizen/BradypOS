#ifndef BRADYPOS_ROOT_RESETS_H
#define BRADYPOS_ROOT_RESETS_H

#include <stdbool.h>

enum reset_components
{
    reset_component_adc,
    reset_component_busctrl,
    reset_component_dma,
    reset_component_i2c0,
    reset_component_i2c1,
    reset_component_io_bank_0,
    reset_component_qspi,
    reset_component_jtag,
    reset_component_pads_bank0,
    reset_component_pads_qspi,
    reset_component_pio0,
    reset_component_pio1,
    reset_component_pll_sys,
    reset_component_pll_usb,
    reset_component_pwm,
    reset_component_rtc,
    reset_component_spi0,
    reset_component_spi1,
    reset_component_syscfg,
    reset_component_sysinfo,
    reset_component_tbman,
    reset_component_timer,
    reset_component_uart0,
    reset_component_uart1,
    reset_component_usbctrl,
};

void disable_component(enum reset_components component);
void enable_component(enum reset_components component);
void reset_component(enum reset_components component);
bool component_ready(enum reset_components component);

#endif
