#include <l4/ipc.h>
#include <vfs/client.h>

bool open_root_fd(L4_thread_id romfs_server, int fd)
{
    L4_msg_tag_t msg_tag = {.flags = 0, .label = VFS_OPENROOT, .t = 0, .u = 1};
    L4_thread_id from;
    L4_load_mr(VFS_OPENROOT_OP, msg_tag.raw);
    L4_load_mr(VFS_OPENROOT_FD, (unsigned)fd);
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

bool open_at(L4_thread_id romfs_server, int fd, int new_fd, const char *path,
             size_t pathlen)
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
    if (L4_ipc_failed(answer_tag) || answer_tag.label != VFS_OPENAT_RET ||
        answer_tag.u != 1 || answer_tag.t != 0)
    {
        return false;
    }
    return true;
}

bool close(L4_thread_id romfs_server, int fd)
{
    L4_load_mr(
        VFS_CLOSE_OP,
        (L4_msg_tag_t){.u = 1, .t = 0, .flags = 0, .label = VFS_CLOSE}.raw);
    L4_load_mr(VFS_CLOSE_FD, fd);
    L4_thread_id from;
    const L4_msg_tag_t answer_tag = L4_ipc(
        romfs_server, romfs_server, L4_timeouts(L4_never, L4_never), &from);
    return !L4_ipc_failed(answer_tag) && answer_tag.label == VFS_CLOSE_RET;
}

struct vfs_stat_result stat(L4_thread_id romfs_server, int fd)
{
    L4_load_mr(
        VFS_STAT_OP,
        (L4_msg_tag_t){.u = 1, .t = 0, .flags = 0, .label = VFS_STAT}.raw);
    L4_load_mr(VFS_STAT_FD, fd);
    L4_thread_id from;
    struct vfs_stat_result result = {
        .success = false, .type = VFS_FT_OTHER, .size = 0};
    const L4_msg_tag_t answer_tag = L4_ipc(
        romfs_server, romfs_server, L4_timeouts(L4_never, L4_never), &from);
    if (L4_ipc_failed(answer_tag) || answer_tag.label != VFS_STAT_RET)
    {
        return result;
    }

    unsigned result_type;
    L4_store_mr(VFS_STAT_RET_TYPE, &result_type);
    result.type = result_type;
    L4_store_mr(VFS_STAT_RET_SIZE, &result.size);
    result.success = true;
    return result;
}

struct map_result map(L4_thread_id romfs_server, int fd, size_t offset,
                      size_t size)
{
    const struct map_result invalid_result = {.fpage = L4_nilpage,
                                              .addr = NULL};
    L4_load_mr(
        VFS_MAP_OP,
        (L4_msg_tag_t){.u = 4, .t = 0, .flags = 0, .label = VFS_MAP}.raw);
    L4_load_mr(VFS_MAP_FD, fd);
    L4_load_mr(VFS_MAP_OFFSET, offset);
    L4_load_mr(VFS_MAP_SIZE, size);
    L4_load_mr(VFS_MAP_PERM, L4_readable);
    L4_thread_id from;
    const L4_msg_tag_t answer_tag = L4_ipc(
        romfs_server, romfs_server, L4_timeouts(L4_never, L4_never), &from);
    if (L4_ipc_failed(answer_tag) || answer_tag.label != VFS_MAP_RET ||
        answer_tag.u != 1 || answer_tag.t != 2)
        return invalid_result;
    struct map_result result = invalid_result;
    struct L4_map_item map_item;
    unsigned address;
    L4_store_mr(VFS_MAP_RET_ADDR, &address);
    L4_store_mrs(VFS_MAP_RET_MAP_ITEM, 2, (unsigned *)&map_item);
    result.addr = (void *)address;
    result.fpage = L4_map_item_snd_fpage(&map_item);
    return result;
}
