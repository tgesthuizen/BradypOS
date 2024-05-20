#include "kern/systhread.h"
#include <kern/debug.h>
#include <kern/interrupts.h>
#include <kern/kalarm.h>
#include <kern/memory.h>
#include <kern/platform.h>
#include <kern/systhread.h>
#include <kern/systick.h>
#include <kern/thread.h>
#include <stdint.h>

int main()
{
    // Interrupts should be disabled. But better be safe than sorry.
    disable_interrupts();
    *(volatile uintptr_t *)(PPB_BASE + VTOR_OFFSET) = (uintptr_t)__vector;
    asm volatile("dsb" ::: "memory");
    systick_init();
    interrupts_init();
    kalarm_init();
    init_thread_system();
    create_sys_threads();
    init_memory();
    enable_interrupts();
    dbg_puts("Interrupts are enabled, we have booted!\n");
    start_scheduling();

    dbg_puts("Kernel is done, bye!\n");
    return 0;
}
