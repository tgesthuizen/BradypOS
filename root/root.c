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

enum
{
    UTCB_ALIGN = 160,
    ROMFS_SERVER_STACK_SIZE = 256
};
static unsigned char romfs_server_stack[ROMFS_SERVER_STACK_SIZE];

int main()
{

    the_kip = L4_kernel_interface(NULL, NULL, NULL);
    unsigned old_ctrl;
    my_thread_id = L4_global_id(0x40, 1);

    starting_time = L4_system_clock();

    // The kernel has cheated a bit when it created us.
    // Add the necessary pages and address space information now in the
    // aftermath.
    if (L4_space_control(
            my_thread_id, 0,
            L4_fpage_add_rights(L4_fpage_log2((unsigned)the_kip, 10),
                                L4_readable),
            L4_fpage_add_rights(L4_fpage_log2((unsigned)&__utcb, 9),
                                L4_readable | L4_writable),
            L4_NILTHREAD, &old_ctrl) != 1)
    {
        kill_root_thread();
    }

    const L4_thread_id romfs_thread_id = L4_global_id(47, 1);
    if (L4_thread_control(romfs_thread_id, my_thread_id, L4_NILTHREAD,
                          my_thread_id,
                          (unsigned char *)&__utcb + UTCB_ALIGN) != 0)
    {
        kill_root_thread();
    }

    unsigned *romfs_sp =
        (unsigned *)(romfs_server_stack + ROMFS_SERVER_STACK_SIZE) - 1;
    *romfs_sp = (unsigned)((unsigned char *)&__utcb + UTCB_ALIGN);

    extern void romfs_start();
    L4_set_msg_tag((L4_msg_tag_t){.u = 2, .t = 0, .flags = 0, .label = 0});
    L4_load_mr(1, (unsigned)&romfs_start);
    L4_load_mr(2, (unsigned)romfs_sp);
    L4_thread_id real_from;
    const L4_msg_tag_t answer_tag =
        L4_ipc(romfs_thread_id, my_thread_id, 0, &real_from);
    if (answer_tag.flags & (1 << 3))
    {
        kill_root_thread();
    }
    while (1)
        L4_yield();
}
