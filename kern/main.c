#include <kern/debug.h>
#include <kern/nvic.h>
#include <kern/platform.h>
#include <kern/systick.h>
#include <stdint.h>

int main()
{
    // Interrupts should be disabled. But better be safe than sorry.
    disable_interrupts();
    *(volatile uintptr_t *)(PPB_BASE + VTOR_OFFSET) = (uintptr_t)__vector;
    systick_init();
    nvic_init();
    enable_interrupts();

    dbg_puts("Interrupts are enabled, we have booted!\n");

    dbg_puts("Resetting...");
    nvic_set_pending(EXC_RESET, true);
    return 0;
}
