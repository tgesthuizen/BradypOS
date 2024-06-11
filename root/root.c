#include "memory.h"
#include <l4/ipc.h>
#include <l4/kip.h>
#include <l4/schedule.h>
#include <l4/space.h>
#include <l4/thread.h>
#include <root.h>
#include <stddef.h>
#include <string.h>

register void *__got_location asm("r9");

__attribute__((naked)) void _start() { __asm__("b main\n\t"); }

L4_kip_t *the_kip;
static L4_clock_t starting_time;
L4_thread_id my_thread_id;
L4_thread_id romfs_thread_id;

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
    ROMFS_SERVER_STACK_SIZE = 256,
    IPC_BUFFER_SIZE = 128,
};
static unsigned char romfs_server_stack[ROMFS_SERVER_STACK_SIZE];
static unsigned char romfs_ipc_buffer[IPC_BUFFER_SIZE];
static L4_msg_buffer_t romfs_msg_buffer;

static L4_msg_buffer_t romfs_construct_message_buffer()
{
    L4_msg_buffer_t res;
    res.raw[0] = L4_string_items_acceptor.raw;
    struct L4_simple_string_item ipc_buffer;
    ipc_buffer.c = 0;
    ipc_buffer.compound = 0;
    ipc_buffer.length = IPC_BUFFER_SIZE;
    ipc_buffer.type = L4_data_type_string_item;
    ipc_buffer.ptr = (unsigned)romfs_ipc_buffer;

    memcpy(res.raw + 1, &ipc_buffer, sizeof(ipc_buffer));
    // TODO Simple string item
    return res;
}

int main()
{

    the_kip = L4_kernel_interface(NULL, NULL, NULL);
    unsigned old_ctrl;
    my_thread_id = L4_global_id(L4_USER_THREAD_START, 1);

    starting_time = L4_system_clock();
    init_memory_management();

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
    *--romfs_sp = (unsigned)((unsigned char *)&__utcb + UTCB_ALIGN);

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

    romfs_msg_buffer = romfs_construct_message_buffer();

    // Handle IPC requests
    while (1)
    {
        L4_load_brs(0, 3, romfs_msg_buffer.raw);
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
