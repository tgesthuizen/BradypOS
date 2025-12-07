#include "clocks.h"
#include "resets.h"

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

static void init_xosc()
{
    volatile unsigned *reg_xosc_control =
        (volatile unsigned *)(XOSC_BASE + xosc_control);
    *reg_xosc_control = xosc_freq_1_15MHZ | (xosc_enable_magic << 12);
    volatile unsigned *reg_xosc_status =
        (volatile unsigned *)(XOSC_BASE + xosc_status);
    // Wait for XOSC to become stable
    while ((*reg_xosc_status & xosc_status_stable) != 1)
        ;
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

void setup_pll(unsigned char *base, unsigned fbdiv, unsigned postdiv1,
               unsigned postdiv2)
{
    // The procedure below is explained in the reference manual of the RP2040.
    // It pays close attention to giving precise instructions, so we better do
    // not deviate from them.

    // Reset PLL and wait for it to become available
    enum reset_components component = (base == (unsigned char *)PLL_USB_BASE)
                                          ? reset_component_pll_usb
                                          : reset_component_pll_sys;
    reset_component(component);
    while (!component_ready(component))
        ;

    // Set VCO values
    *(volatile unsigned *)(base + pll_cs) = (1 << pll_cs_refdiv_low);
    *(volatile unsigned *)(base + pll_fbdiv_int) = fbdiv;

    // Turn on VCO power
    *(volatile unsigned *)(base + pll_pwr) &=
        ~((1 << pll_pwr_pd) | (1 << pll_pwr_vcopd));
    // Wait for the PLL to lock
    while ((*(volatile unsigned *)(base + pll_cs) & (1 << pll_cs_lock)) != 0)
        ;

    // Setup post dividers
    *(volatile unsigned *)(base + pll_prim) =
        (postdiv1 << pll_prim_postdiv1_low) |
        (postdiv2 << pll_prim_postdiv2_low);

    // Turn on post dividing
    *(volatile unsigned *)(base + pll_pwr) &= ~(1 << pll_pwr_postdivp);
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

static void setup_plls()
{
    setup_pll((unsigned char *)PLL_SYS_BASE, PLL_SYS_DEFAULT_FBDIV,
              PLL_SYS_DEFAULT_POSTDIV1, PLL_SYS_DEFAULT_POSTDIV2);
    setup_pll((unsigned char *)PLL_USB_BASE, PLL_USB_DEFAULT_FBDIV,
              PLL_USB_DEFAULT_POSTDIV1, PLL_USB_DEFAULT_POSTDIV2);
}

#define CLOCKS_BASE 0x40008000

struct rp2040_clock_t
{
    unsigned ctrl;
    unsigned div;
    unsigned selected;
};

#define CLOCKS ((volatile struct rp2040_clock_t *)CLOCKS_BASE)

bool has_glitchless_mux(enum clock_index clk);

bool clock_configure(enum clock_index idx, unsigned src, unsigned auxsrc,
                     unsigned div)
{
    volatile struct rp2040_clock_t *const clock = CLOCKS + idx;

    // When we step up the divisor, do so right away.
    // This is done so that we do not output a high-frequency clock signal when
    // switching to a larger divider and a faster clock.
    if (div > clock->div)
        clock->div = div;
}

void setup_clocks()
{
    // TODO: Assign XOSC to PLLs before setting them up.
    // Configure clocks afterwards, check out manual 2.15 Clocks.
    init_xosc();
    setup_plls();
}
