#include "config.h"
#include <l4/ipc.h>
#include <l4/schedule.h>
#include <string.h>
#include <vfs.h>

enum
{
    IPC_BUFFER_SIZE = 128,
};

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

enum
{
    ROOT_FD = 3,
    ETC_FD = 4,
    INITTAB_FD = 5,
};

static bool open_root_fd(L4_thread_id romfs_server)
{
    L4_msg_tag_t msg_tag = {.flags = 0, .label = VFS_OPENROOT, .t = 0, .u = 1};
    L4_thread_id from;
    L4_load_mr(VFS_OPENROOT_OP, msg_tag.raw);
    L4_load_mr(VFS_OPENROOT_FD, (unsigned)ROOT_FD);
    L4_msg_tag_t answer_tag = L4_ipc(romfs_server, romfs_server,
                                     L4_timeouts(L4_never, L4_never), &from);
    if (L4_ipc_failed(answer_tag))
    {
        return false;
    }
    if (answer_tag.label != VFS_OPENROOT_RET)
    {
        return false;
    }
    if (answer_tag.u != 1 || answer_tag.t != 0)
    {
        return false;
    }

    return true;
}

static bool open_at(L4_thread_id romfs_server, int fd, int new_fd,
                    const char *path, size_t pathlen)
{
    const L4_msg_tag_t msg_tag = {
        .flags = 0, .label = VFS_OPENAT, .u = 2, .t = 2};
    const struct L4_simple_string_item ssi = {.c = 0,
                                              .compound = 0,
                                              .type = L4_data_type_string_item,
                                              .length = pathlen,
                                              .ptr = (unsigned)path};
    L4_thread_id from;
    L4_load_mr(VFS_OPENAT_OP, msg_tag.raw);
    L4_load_mr(VFS_OPENAT_FD, fd);
    L4_load_mr(VFS_OPENAT_NEWFD, new_fd);
    L4_load_mrs(VFS_OPENAT_STR, 2, (unsigned *)&ssi);
    const L4_msg_tag_t answer_tag = L4_ipc(
        romfs_server, romfs_server, L4_timeouts(L4_never, L4_never), &from);
    if (L4_ipc_failed(answer_tag) || answer_tag.label != VFS_OPENAT ||
        answer_tag.u != 1 || answer_tag.t != 0)
    {
        return false;
    }
    return true;
}

void parse_init_config(L4_thread_id romfs_server)
{
    open_root_fd(romfs_server);
    static const char etc_path[] = "etc";
    open_at(romfs_server, ROOT_FD, ETC_FD, etc_path, sizeof(etc_path) - 1);
    static const char inittab_path[] = "inittab";
    open_at(romfs_server, ETC_FD, INITTAB_FD, inittab_path,
            sizeof(inittab_path) - 1);
}
