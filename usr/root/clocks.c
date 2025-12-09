#include "clocks.h"
#include "resets.h"

#include <l4/schedule.h>
#include <stdint.h>

static inline void mmio_write32(uintptr_t addr, uint32_t val)
{
    *(volatile uint32_t *)addr = val;
}
static inline uint32_t mmio_read32(uintptr_t addr)
{
    return *(volatile uint32_t *)addr;
}
static inline volatile uint32_t *mmio_addr32(uintptr_t addr)
{
    return (volatile uint32_t *)addr;
}

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
    mmio_write32(XOSC_BASE + xosc_control,
                 xosc_freq_1_15MHZ | (xosc_enable_magic << 12));
    while ((mmio_read32(XOSC_BASE + xosc_status) >> xosc_status_stable) != 1)
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
    // 1. Reset PLL
    enum reset_components component =
        (base == PLL_USB_BASE ? reset_component_pll_usb
                              : reset_component_pll_sys);
    reset_component(component);
    while (!component_ready(component))
        L4_yield();

    // 2. Power down PLL
    mmio_write32(base + pll_pwr, (1 << pll_pwr_pd) | (1 << pll_pwr_vcopd) |
                                     (1 << pll_pwr_postdivp));

    // 3. Configure refdiv (keep other bits unchanged)
    mmio_write32(base + pll_cs, (1 << pll_cs_refdiv_low));

    // 4. Configure fbdiv
    mmio_write32(base + pll_fbdiv_int, fbdiv);

    // 5. Power up VCO
    uint32_t pwr = mmio_read32(base + pll_pwr);
    pwr &= ~((1 << pll_pwr_pd) | (1 << pll_pwr_vcopd));
    mmio_write32(base + pll_pwr, pwr);

    // 6. Wait for lock
    while (!(mmio_read32(base + pll_cs) & (1 << pll_cs_lock)))
        L4_yield();

    // 7. Configure postdivs
    mmio_write32(base + pll_prim, (postdiv1 << pll_prim_postdiv1_low) |
                                      (postdiv2 << pll_prim_postdiv2_low));

    // 8. Enable postdiv stage **only this bit is touched**
    pwr = mmio_read32(base + pll_pwr);
    pwr &= ~(1 << pll_pwr_postdivp);
    mmio_write32(base + pll_pwr, pwr);
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

void setup_clocks()
{
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
    uint32_t clk_sys_ctrl =
        (CLOCKS[clk_sys].ctrl & ~0x7E0) | (CLOCK_SYS_AUXSRC_PLL_SYS << 5);
    CLOCKS[clk_sys].ctrl = clk_sys_ctrl;
    clk_sys_ctrl &= ~0x3;
    clk_sys_ctrl |= 1;
    CLOCKS[clk_sys].ctrl = clk_sys_ctrl;
    while (!(CLOCKS[clk_sys].selected & 2))
        L4_yield();

    // Switch clk_peri to PLL_SYS
    CLOCKS[clk_peri].ctrl = 0;
    CLOCKS[clk_peri].ctrl = (CLOCK_PERI_AUXSRC_PLL_SYS << 5) | (1 << 11);
}
