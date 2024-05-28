#include <kern/debug.h>
#include <kern/interrupts.h>
#include <kern/thread.h>
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

static __attribute__((used)) void __isr_hardfault(unsigned lr)
{
    dbg_log(DBG_INTERRUPT, "Executing HardFault\n");

    bool fault_in_thread_mode = (lr & (1 << 3)) != 0;
    bool uses_psp = (lr & (1 << 2)) != 0;
    unsigned *faulted_sp = uses_psp ? get_psp() : get_msp();
    dbg_printf(
        "Context stored to %s\n"
        "mode:\t%s\n"
        "SP:\t0x%08x\n"
        "PSR:\t0x%08x\n"
        "r0:\t0x%08x\n"
        "r1:\t0x%08x\n"
        "r2:\t0x%08x\n"
        "r3:\t0x%08x\n"
        "r12:\t0x%08x\n"
        "lr:\t0x%08x\n"
        "pc:\t0x%08x\n",
        uses_psp ? "PSP" : "MSP", fault_in_thread_mode ? "thread" : "handler",
        (unsigned)faulted_sp, faulted_sp[THREAD_CTX_STACK_PSR],
        faulted_sp[THREAD_CTX_STACK_R0], faulted_sp[THREAD_CTX_STACK_R1],
        faulted_sp[THREAD_CTX_STACK_R2], faulted_sp[THREAD_CTX_STACK_R3],
        faulted_sp[THREAD_CTX_STACK_R12], faulted_sp[THREAD_CTX_STACK_LR],
        faulted_sp[THREAD_CTX_STACK_PC]);

    panic("!!! PANIC: Hardfault triggered\n");
}

__attribute__((naked)) void isr_hardfault()
{
    asm("movs r0, r9\n\t"
        "movs r1, lr\n\t"
        "push {r0, lr}\n\t"
#ifndef NDEBUG
        ".cfi_def_cfa_offset 8\n\t"
        ".cfi_offset 9, -8\n\t"
        ".cfi_offset 14, -4\n\t"
#endif
        "ldr r0, =0x20040000\n\t"
        "ldr r0, [r0]\n\t"
        "movs r9, r0\n\t"
        "movs r0, r1\n\t"
        "bl __isr_hardfault\n\t"
        "pop {r0}\n\t"
        "movs r9, r0\n\t"
        "pop {pc}\n\t");
}
