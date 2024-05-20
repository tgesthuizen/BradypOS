#include <l4/ipc.h>
#include <l4/kip.h>
#include <l4/schedule.h>
#include <l4/space.h>
#include <l4/thread.h>
#include <stddef.h>

__attribute__((naked)) void _start() { __asm__("b main\n\t"); }

kip_t *the_kip;
static L4_clock_t starting_time;
static L4_thread_id my_thread_id;

// This is our method of indicating a failure in the root thread startup.
// We can't do much, but killing the root thread will make the kernel panic.
static void kill_root_thread()
{
    while (1)
        L4_thread_control(my_thread_id, L4_NILTHREAD, L4_NILTHREAD,
                          L4_NILTHREAD, (void *)(unsigned)-1);
}

int main()
{

    the_kip = L4_kernel_interface(NULL, NULL, NULL);
    unsigned old_ctrl;
    my_thread_id = L4_global_id(46, 1);

    starting_time = L4_system_clock();

    // The kernel has cheated a bit when it created us.
    // Add the necessary pages and address space information now in the
    // aftermath.
    if (L4_space_control(
            L4_my_global_id(), 0, L4_fpage_log2((unsigned)the_kip, 10),
            L4_fpage_log2((unsigned)&__utcb, 9), L4_NILTHREAD, &old_ctrl) != 1)
    {
        kill_root_thread();
    }

    const L4_thread_id romfs_thread_id = L4_global_id(47, 1);
    if (L4_thread_control(romfs_thread_id, my_thread_id, L4_NILTHREAD,
                          my_thread_id, (unsigned char *)&__utcb + 160) != 0)
    {
        kill_root_thread();
    }

    while (1)
        L4_yield();
}
