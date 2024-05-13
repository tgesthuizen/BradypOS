#include <l4/kip.h>
#include <l4/schedule.h>
#include <stddef.h>

__attribute__((naked)) void _start() { __asm__("b main\n\t"); }

static kip_t *the_kip;

int main()
{
    the_kip = L4_kernel_interface(NULL, NULL, NULL);

    while (1)
        L4_yield();
}
