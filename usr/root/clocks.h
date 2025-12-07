#ifndef BRADYPOS_ROOT_CLOCKS_H
#define BRADYPOS_ROOT_CLOCKS_H

#include <stdbool.h>

void setup_pll(unsigned char *base, unsigned fbdiv, unsigned postdiv1,
               unsigned postdiv2);

enum clock_index
{
    clk_gpout0,
    clk_gpout1,
    clk_gpout2,
    clk_gpout3,
    clk_ref,
    clk_sys,
    clk_peri,
    clk_usb,
    clk_adc,
    clk_rtc,
    CLK_COUNT,
};

inline bool has_glitchless_mux(enum clock_index clk)
{
    return clk == clk_sys || clk == clk_ref;
}

void setup_clocks();

#endif
