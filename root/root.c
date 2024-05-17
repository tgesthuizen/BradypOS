#include "l4/thread.h"
#include <l4/kip.h>
#include <l4/schedule.h>
#include <l4/space.h>
#include <stddef.h>

__attribute__((naked)) void _start() { __asm__("b main\n\t"); }

static kip_t *the_kip;
static L4_clock_t starting_time;

int main()
{
    the_kip = L4_kernel_interface(NULL, NULL, NULL);
    unsigned old_ctrl;
    // The kernel has cheated a bit when it created us.
    // Add the necessary pages and address space information now in the
    // aftermath.
    if (L4_space_control(
            L4_my_global_id(), 0, L4_fpage_log2((unsigned)the_kip, 10),
            L4_fpage_log2((unsigned)&__utcb, 9), L4_NILTHREAD, &old_ctrl) != 1)
    {
        // TODO: Delete the root thread as a response, making the kernel panic.
    }

    starting_time = L4_system_clock();

    while (1)
        L4_yield();
}
