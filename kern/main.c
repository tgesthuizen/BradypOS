#include <kern/debug.h>
#include <kern/interrupts.h>
#include <kern/platform.h>
#include <kern/systick.h>
#include <stdint.h>

int main()
{
    // Interrupts should be disabled. But better be safe than sorry.
    disable_interrupts();
    *(volatile uintptr_t *)(PPB_BASE + VTOR_OFFSET) = (uintptr_t)__vector;
    systick_init();
    interrupts_init();
    enable_interrupts();

    dbg_puts("Interrupts are enabled, we have booted!\n");

    dbg_puts("Resetting...");
    reset_processor();
    return 0;
}
