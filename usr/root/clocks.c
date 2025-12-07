#include "clocks.h"
#include "resets.h"

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

static void init_xosc()
{
    volatile unsigned *reg_xosc_control =
        (volatile unsigned *)(XOSC_BASE + xosc_control);
    *reg_xosc_control = xosc_freq_1_15MHZ | (xosc_enable_magic << 12);
    volatile unsigned *reg_xosc_status =
        (volatile unsigned *)(XOSC_BASE + xosc_status);
    // Wait for XOSC to become stable
    while ((*reg_xosc_status >> xosc_status_stable) != 1)
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
        L4_yield();

    // Set VCO values
    *(volatile unsigned *)(base + pll_cs) = (1 << pll_cs_refdiv_low);
    *(volatile unsigned *)(base + pll_fbdiv_int) = fbdiv;

    // Turn on VCO power
    *(volatile unsigned *)(base + pll_pwr) &=
        ~((1 << pll_pwr_pd) | (1 << pll_pwr_vcopd));
    // Wait for the PLL to lock
    while ((*(volatile unsigned *)(base + pll_cs) & (1 << pll_cs_lock)) == 0)
        L4_yield();

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

#define CLOCKS_BASE 0x40008000

struct rp2040_clock_t
{
    unsigned ctrl;
    unsigned div;
    unsigned selected;
};

#define CLOCKS ((volatile struct rp2040_clock_t *)CLOCKS_BASE)

bool has_glitchless_mux(enum clock_index clk);

/* Helper mmio */
static inline void mmio_write32(uintptr_t addr, uint32_t val)
{
    *(volatile uint32_t *)addr = val;
}
static inline uint32_t mmio_read32(uintptr_t addr)
{
    return *(volatile uint32_t *)addr;
}

/* CLOCKS register offsets (from RP2040) */
#define CLK_REF_CTRL_OFFSET 0x30u
#define CLK_SYS_CTRL_OFFSET 0x3Cu
#define CLK_PERI_CTRL_OFFSET 0x48u
#define CLK_USB_CTRL_OFFSET 0x54u

#define CLK_CTRL_AUXSRC_SHIFT 5u
#define CLK_CTRL_ENABLE_BIT (1u << 11u)

#define CLK_PERI_CTRL_ADDR (CLOCKS_BASE + CLK_PERI_CTRL_OFFSET)
#define CLK_REF_CTRL_ADDR (CLOCKS_BASE + CLK_REF_CTRL_OFFSET)

/* AUXSRC values we need (from Pico SDK)
 *   PLL_USB = 0x2
 *   XOSC    = 0x4  (if you ever want to pick XOSC)
 */
#define CLK_AUXSRC_PLL_USB (0x2u)
#define CLK_AUXSRC_XOSC (0x4u)

/* Minimal function that routes clk_peri to PLL_USB and enables it */
static void route_peri_to_pll_usb(void)
{
    /* Compose control value: AUXSRC = PLL_USB (shifted) | ENABLE */
    unsigned ctrl =
        (CLK_AUXSRC_PLL_USB << CLK_CTRL_AUXSRC_SHIFT) | CLK_CTRL_ENABLE_BIT;
    mmio_write32(CLK_PERI_CTRL_ADDR, ctrl);
}

/* If you want to be explicit about clk_ref being XOSC (optional but clean) */
static void route_ref_to_xosc(void)
{
    unsigned ctrl =
        (CLK_AUXSRC_XOSC << CLK_CTRL_AUXSRC_SHIFT) | CLK_CTRL_ENABLE_BIT;
    mmio_write32(CLK_REF_CTRL_ADDR, ctrl);

    L4_yield();
}

/* ----- top-level setup_clocks (minimal) ----- */
void setup_clocks(void)
{
    /* 1) bring up XOSC */
    init_xosc();

    /* Optionally route clk_ref to XOSC so PLLs clearly see correct ref */
    route_ref_to_xosc();

    /* 2) program PLLs (SYS and USB) using your defaults */
    setup_pll((unsigned char *)PLL_SYS_BASE, PLL_SYS_DEFAULT_FBDIV,
              PLL_SYS_DEFAULT_POSTDIV1, PLL_SYS_DEFAULT_POSTDIV2);

    setup_pll((unsigned char *)PLL_USB_BASE, PLL_USB_DEFAULT_FBDIV,
              PLL_USB_DEFAULT_POSTDIV1, PLL_USB_DEFAULT_POSTDIV2);

    /* 3) route clk_peri to PLL_USB so peripherals (UART) run at 48 MHz */
    route_peri_to_pll_usb();

    /* done - minimal and focused on UART stability */
}
