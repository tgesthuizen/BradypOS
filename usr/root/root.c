#include "config.h"
#include "memory.h"
#include <errno.h>
#include <l4/ipc.h>
#include <l4/kip.h>
#include <l4/schedule.h>
#include <l4/space.h>
#include <l4/thread.h>
#include <root.h>
#include <root/service.h>
#include <service.h>
#include <stddef.h>
#include <string.h>

register void *__got_location asm("r9");

L4_kip_t *the_kip;
static L4_clock_t starting_time;
L4_thread_id my_thread_id;
L4_thread_id romfs_thread_id;
struct L4_utcb_t *romfs_server_utcb;
unsigned next_thread_no;

// This is our method of indicating a failure in the root thread startup.
// We can't do much, but killing the root thread will make the kernel panic.
void kill_root_thread()
{
    while (1)
        L4_thread_control(my_thread_id, L4_NILTHREAD, L4_NILTHREAD,
                          L4_NILTHREAD, (void *)(unsigned)-1);
}

enum
{
    UTCB_ALIGN = 160,
    ROMFS_SERVER_STACK_SIZE = 512,
};
static unsigned char romfs_server_stack[ROMFS_SERVER_STACK_SIZE];

__attribute__((naked, section(".text.startup"))) void _start()
{
    __asm__("b main\n\t");
}

__attribute__((section(".text.startup"))) int main()
{

    the_kip = L4_kernel_interface(NULL, NULL, NULL);
    unsigned old_ctrl;
    my_thread_id = L4_global_id(L4_USER_THREAD_START, 1);

    starting_time = L4_system_clock();
    init_memory_management();

    register_service_from_name(my_thread_id, "mem\0");
    register_service_from_name(my_thread_id, "serv");
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

    romfs_thread_id = L4_global_id(L4_USER_THREAD_START + 1, 1);
    romfs_server_utcb =
        (struct L4_utcb_t *)((unsigned char *)&__utcb + UTCB_ALIGN);
    if (L4_thread_control(romfs_thread_id, my_thread_id, my_thread_id,
                          my_thread_id,
                          (unsigned char *)&__utcb + UTCB_ALIGN) != 1)
    {
        kill_root_thread();
    }
    if (L4_set_priority(romfs_thread_id, 60) == L4_tstate_error)
    {
        kill_root_thread();
    }

    unsigned *romfs_sp =
        (unsigned *)(romfs_server_stack + ROMFS_SERVER_STACK_SIZE);
    *--romfs_sp = (unsigned)__got_location;
    *--romfs_sp = (unsigned)romfs_server_utcb;

    extern void romfs_start();
    L4_set_msg_tag((L4_msg_tag_t){.u = 2, .t = 0, .flags = 0, .label = 0});
    L4_load_mr(1, (unsigned)&romfs_start);
    L4_load_mr(2, (unsigned)romfs_sp);
    L4_thread_id real_from;
    const L4_msg_tag_t answer_tag =
        L4_ipc(romfs_thread_id, L4_NILTHREAD, 0, &real_from);
    if (L4_ipc_failed(answer_tag))
    {
        kill_root_thread();
    }
    register_service_from_name(romfs_thread_id, "rom\0");

    next_thread_no = L4_USER_THREAD_START + 2;

    parse_init_config(romfs_thread_id);

    // Handle IPC requests
    while (1)
    {
        L4_thread_id from;
        const L4_msg_tag_t msg_tag = L4_ipc(
            L4_NILTHREAD, L4_ANYTHREAD, L4_timeouts(L4_never, L4_never), &from);
        if (L4_ipc_failed(msg_tag))
        {
            // TODO: Properly log error
            continue;
        }
        switch (msg_tag.label)
        {
        case ROOT_ALLOC_MEM:
            handle_ipc_alloc(msg_tag);
            break;
        case ROOT_FREE_MEM:
            handle_ipc_free(msg_tag);
            break;
        case SERV_GET:
            handle_service_get(msg_tag, from);
            break;
        case SERV_REGISTER:
            handle_service_register(msg_tag, from);
            break;
        default:
            make_service_ipc_error(EINVAL);
            break;
        }
        const L4_msg_tag_t answer_tag = L4_ipc(
            from, L4_NILTHREAD, L4_timeouts(L4_zero_time, L4_zero_time), &from);
        if (L4_ipc_failed(answer_tag))
        {
            // TODO: Properly log error
            continue;
        }
    }
}
