
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

void setup_clocks() { init_xosc(); }
