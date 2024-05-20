#include <kern/debug.h>
#include <kern/systick.h>

struct systick_t
{
    unsigned csr;
    unsigned rvr;
    unsigned cvr;
    unsigned calib;
};

// This address is mandated by the ARMv6m documentation
#define SYSTICK ((volatile struct systick_t *)0xE000E010)

enum systick_csr_bits
{
    SYSTICK_CSR_ENABLE = (1 << 0),
    SYSTICK_CSR_TICKINT = (1 << 1),
    SYSTICK_CSR_CLKSRC = (1 << 2),
    SYSTICK_CSR_COUNTFLAG = (1 << 16),
};

enum systick_calib_bits
{
    SYSTICK_CALIB_NOREF = (1 << 31),
    SYSTICK_CALIB_SKEW = (1 << 30),
    SYSTICK_CALIB_TENMS_MASK = (1 << 24) - 1,
};

void systick_init()
{
    // Explicitly disable the systick timer
    const unsigned control_flags =
        SYSTICK_CSR_TICKINT | SYSTICK_CSR_CLKSRC | SYSTICK_CSR_COUNTFLAG;
    SYSTICK->csr = control_flags;
    // Fire every 100ms
    const unsigned tick_rate = SYSTICK->calib & (SYSTICK_CALIB_TENMS_MASK);
    SYSTICK->rvr = tick_rate;
    SYSTICK->cvr = tick_rate;
    SYSTICK->csr = control_flags | SYSTICK_CSR_ENABLE;
}
