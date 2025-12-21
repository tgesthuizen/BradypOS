#include "clocks.h"
#include <rp2040/bus_fabric.h>
#include <rp2040/resets.h>

#include <l4/schedule.h>
#include <stdint.h>

#define XOSC_BASE 0x40024000

enum xosc_register_offsets
{
    xosc_control = 0x00,
    xosc_status = 0x04,
    xosc_dormant = 0x08,
    xosc_startup = 0x0c,
    xosc_count = 0x1c,
};

enum xosc_frequencies
{
    xosc_freq_1_15MHZ = 0xaa0,
    xosc_freq_reserved_1,
    xosc_freq_reserved_2,
    xosc_freq_reserved_3,
};

enum xosc_enable_values
{
    xosc_enable_magic = 0xfab,
    xosc_disable_magic = 0xd1e,
};

enum xosc_status_bits
{
    xosc_status_enabled = 12,
    xosc_status_badwrite = 24,
    xosc_status_stable = 31,
};

enum
{
    xosc_12MHz_wait_count = 47,
};

void init_xosc()
{
    mmio_write32(XOSC_BASE + xosc_control, xosc_freq_1_15MHZ);
    // The RP2040 documentation says this is an adequate wait interval for 12
    // MHz
    mmio_write32(XOSC_BASE + xosc_count, xosc_12MHz_wait_count);

    mmio_set32(XOSC_BASE + xosc_control, xosc_enable_magic << 12);
    while (!(mmio_read32(XOSC_BASE + xosc_status) & (1 << xosc_status_stable)))
        L4_yield();
}

#define PLL_SYS_BASE 0x40028000
#define PLL_USB_BASE 0x4002c000

enum pll_register_offsets
{
    pll_cs = 0x0,
    pll_pwr = 0x4,
    pll_fbdiv_int = 0x8,
    pll_prim = 0xc,
};

enum pll_cs_bits
{
    pll_cs_refdiv_low = 0,
    pll_cs_refdiv_high = 5,
    pll_cs_bypass = 8,
    pll_cs_lock = 31,
};

enum pll_pwr_bits
{
    pll_pwr_pd = 0,
    pll_pwr_dsmpd = 2,
    pll_pwr_postdivp = 3,
    pll_pwr_vcopd = 5,
};

enum pll_prim_bits
{
    pll_prim_postdiv2_low = 12,
    pll_prim_postdiv2_high = 14,
    pll_prim_postdiv1_low = 16,
    pll_prim_postdiv1_high = 18,
};

void setup_pll(uintptr_t base, unsigned fbdiv, unsigned postdiv1,
               unsigned postdiv2)
{
    enum reset_components component =
        (base == PLL_USB_BASE ? reset_component_pll_usb
                              : reset_component_pll_sys);
    reset_component(component);
    while (!component_ready(component))
        L4_yield();

    mmio_write32(base + pll_cs, 1); // Set refdiv
    mmio_write32(base + pll_fbdiv_int, fbdiv);

    // TODO: Shift proper values here
    uint32_t power = (1 << pll_pwr_pd) | (1 << pll_pwr_vcopd);

    mmio_clear32(base + pll_pwr, power);
    while (!(mmio_read32(base + pll_cs) & (1 << pll_cs_lock)))
        L4_yield();

    uint32_t pdiv = (postdiv1 << pll_prim_postdiv1_low) |
                    (postdiv2 << pll_prim_postdiv2_low);
    mmio_write32(base + pll_prim, pdiv);
    mmio_clear32(base + pll_pwr, 1 << pll_pwr_postdivp);
}

// These constants are shamelessly stolen from the rpi-pico SDK.
enum default_pll_values
{
    PLL_SYS_DEFAULT_FBDIV = 125,
    PLL_SYS_DEFAULT_POSTDIV1 = 6,
    PLL_SYS_DEFAULT_POSTDIV2 = 2,
    PLL_USB_DEFAULT_FBDIV = 100,
    PLL_USB_DEFAULT_POSTDIV1 = 5,
    PLL_USB_DEFAULT_POSTDIV2 = 5,
};

#define CLOCKS_BASE 0x40008000

struct rp2040_clock_t
{
    unsigned ctrl;
    unsigned div;
    unsigned selected;
};

#define CLOCKS ((volatile struct rp2040_clock_t *)CLOCKS_BASE)

enum clock_sys_auxsrc
{
    CLOCK_SYS_AUXSRC_PLL_SYS,
    CLOCK_SYS_AUXSRC_PLL_USB,
    CLOCK_SYS_AUXSRC_ROSC,
    CLOCK_SYS_AUXSRC_XOSC,
    CLOCK_SYS_AUXSRC_GPIN0,
    CLOCK_SYS_AUXSRC_GPIN1,
};

enum clock_sys_src
{
    CLOCK_SYS_SRC_CLK_REF,
    CLOCK_SYS_SRC_CLK_SYS_AUX,
};

enum clk_peri_auxsrc
{
    CLOCK_PERI_AUXSRC_CLK_SYS,
    CLOCK_PERI_AUXSRC_PLL_SYS,
    CLOCK_PERI_AUXSRC_PLL_USB,
    CLOCK_PERI_AUXSRC_ROSC,
    CLOCK_PERI_AUXSRC_XOSC,
    CLOCK_PERI_AUXSRC_GPIN0,
    CLOCK_PERI_AUXSRC_GPIN1,
};

enum clk_ref_ctrl_bits
{
    clk_ref_ctrl_src_low = 0,
    clk_ref_ctrl_src_high = 1,
    clk_ref_ctrl_auxsrc_low = 5,
    clk_ref_ctrl_auxsrc_high = 6,
};

enum clk_sys_ctrl_bits
{
    clk_sys_ctrl_src = 0,
    clk_sys_ctrl_auxsrc_low = 5,
    clk_sys_ctrl_auxsrc_high = 7,
};

enum clk_gpout0_ctrl_bits
{
    clk_gpout0_ctrl_auxsrc_low = 5,
    clk_gpout0_ctrl_auxsrc_high = 8,
    clk_gpout0_ctrl_kill = 10,
    clk_gpout0_ctrl_enable = 11,
    clk_gpout0_ctrl_dc50 = 12,
    clk_gpout0_ctrl_phase_low = 16,
    clk_gpout0_ctrl_phase_high = 17,
    clk_gpout0_ctrl_nudge = 20,
};

enum clk_gpout0_div_bits
{
    clk_gpout0_div_frac_low = 0,
    clk_gpout0_div_frac_high = 7,
    clk_gpout0_div_int_low = 8,
    clk_gpout0_div_int_high = 31,
};

static inline void busy_wait_at_least_cycles(uint32_t minimum_cycles)
{
    asm volatile(".syntax unified\n"
                 "1: subs %0, #3\n"
                 "bcs 1b\n"
                 : "+l"(minimum_cycles)
                 :
                 : "cc", "memory");
}

static bool configure_clock(enum clock_index idx, uint32_t src, uint32_t auxsrc,
                            uint32_t src_freq, uint32_t freq)
{
    uint32_t div;

    // assert(src_freq >= freq);

    if (freq > src_freq)
        return false;
    div = (uint32_t)(((uint64_t)src_freq << clk_gpout0_div_int_low) / freq);
    volatile struct rp2040_clock_t *clock = &CLOCKS[idx];
    if (div > clock->div)
    {
        clock->div = div;
    }
    if (has_glitchless_mux(idx) && src == CLOCK_SYS_SRC_CLK_SYS_AUX)
    {
        mmio_clear32((uintptr_t)&clock->ctrl, 0b11 << clk_ref_ctrl_src_low);
        while (!(clock->selected & 1u))
            L4_yield();
    }
    else
    {
        mmio_clear32((uintptr_t)&clock->ctrl, (1 << clk_gpout0_ctrl_enable));
        unsigned delay_cyc =
            (configured_freq[clk_sys] / configured_freq[idx]) + 1;
        busy_wait_at_least_cycles(delay_cyc);
    }
    mmio_write_masked32((uintptr_t)&clock->ctrl,
                        auxsrc << clk_sys_ctrl_auxsrc_low,
                        0b111 << clk_sys_ctrl_auxsrc_low);
    if (has_glitchless_mux(idx))
    {
        mmio_write_masked32((uintptr_t)&clock->ctrl,
                            src << clk_ref_ctrl_src_low,
                            0b11 << clk_ref_ctrl_src_low);
        while (!(clock->selected & (1u << src)))
            L4_yield();
    }

    mmio_set32((uintptr_t)&clock->ctrl, (1 << clk_gpout0_ctrl_enable));
    clock->div = div;
    // configured_freq[clk_index] = (uint32_t)(((uint64_t) src_freq << 8) /
    // div);
    return true;
}

void setup_clocks()
{
    if (CLOCKS[clk_sys].selected & (1 << CLOCK_SYS_SRC_CLK_SYS_AUX))
    {
        // It seems like clocks are already setup - skip initialization
        // TODO: Properly investigate why the code below fails when clocks are
        // already active. It's probably easily mitigated.
        return;
    }

    init_xosc();
    // Switch clk_ref to xosc
    CLOCKS[clk_ref].ctrl = 2;
    // Wait until the switch has happened
    while (((CLOCKS[clk_ref].selected >> 2) & 1) == 0)
        L4_yield();
    setup_pll(PLL_SYS_BASE, PLL_SYS_DEFAULT_FBDIV, PLL_SYS_DEFAULT_POSTDIV1,
              PLL_SYS_DEFAULT_POSTDIV2);
    setup_pll(PLL_USB_BASE, PLL_USB_DEFAULT_FBDIV, PLL_USB_DEFAULT_POSTDIV1,
              PLL_USB_DEFAULT_POSTDIV2);

    // Switch clk_sys to PLL_SYS
    enum
    {
        MHz12 = 12000000,
        MHz125 = 125000000,
    };
    configure_clock(clk_sys, CLOCK_SYS_SRC_CLK_SYS_AUX,
                    CLOCK_SYS_AUXSRC_PLL_SYS, MHz125, MHz125);
    configure_clock(clk_peri, 0, CLOCK_PERI_AUXSRC_PLL_SYS, MHz125, MHz125);
}
