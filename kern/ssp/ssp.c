#include <kern/debug.h>
#include <stdint.h>

const uintptr_t __stack_chk_guard = 0xe2dee396;

__attribute__((noreturn)) void __stack_chk_fail(void)
{
    panic("Smashed stack detected!");
}
