#include <kern/debug.h>
#include <kern/interrupts.h>
#include <semihosting.h>
#include <stdarg.h>
#include <tinyprintf.h>

#ifndef NO_DEBUG_LOG
char debug_should_log[DBG_CATEGORY_LAST];
char debug_buf[128];
#endif

void dbg_puts(const char *str) { sh_write0(str); }

static void dbg_vprintf(const char *fmt, va_list args)
{
    tfp_vsprintf(debug_buf, fmt, args);
    sh_write0(debug_buf);
}

void dbg_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    dbg_vprintf(fmt, args);
    va_end(args);
}

// TODO: Make this reusable and move it somewhere in the platform code.
#include <kern/platform.h>

enum
{
    AIRCR_VECTKEY_MAGIC = 0x05fa << 16,
    AIRCR_SYSRESETREQ = 1 << 2,
};

void reset_processor()
{
    // Request the reset
    *(volatile uint32_t *)(PPB_BASE + AIRCR_OFFSET) =
        AIRCR_VECTKEY_MAGIC | AIRCR_SYSRESETREQ;
    // And wait for it to happen.
    // The armv6m reference manual explicitly says that the reset might
    // take some time to happen.
    // TODO: Should we disable interrupts here?
    // BUG: Turns out the reset can take quite some time. I don't think what we
    // do here is sufficient to cause a reset.
    asm volatile("dsb" ::: "memory");
    while (1)
        asm volatile("wfi");
}

void panic(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    dbg_vprintf(fmt, args);
    va_end(args);

    reset_processor();
}
