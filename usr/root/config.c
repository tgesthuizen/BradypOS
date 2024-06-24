#include "config.h"
#include "exec.h"
#include <l4/ipc.h>
#include <l4/schedule.h>
#include <l4/space.h>
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

void kill_root_thread();

enum
{
    ROOT_FD = 3,
    ETC_FD = 4,
    INITTAB_FD = 5,
    OTHER_FD_START,
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
    if (L4_ipc_failed(answer_tag) || answer_tag.label != VFS_OPENAT_RET ||
        answer_tag.u != 1 || answer_tag.t != 0)
    {
        return false;
    }
    return true;
}

static bool close(L4_thread_id romfs_server, int fd)
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

struct vfs_stat_result
{
    bool success;
    enum vfs_file_type type;
    size_t size;
};

static struct vfs_stat_result stat(L4_thread_id romfs_server, int fd)
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

struct map_result
{
    L4_fpage_t fpage;
    void *addr;
};

static struct map_result map(L4_thread_id romfs_server, int fd, size_t offset,
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

static void swap_fds(int *fda, int *fdb)
{
    int tmp = *fda;
    *fda = *fdb;
    *fdb = (tmp == ROOT_FD) ? OTHER_FD_START : tmp;
}

static void parse(L4_thread_id romfs_server, const char *str_start,
                  const char *end)
{
    enum parser_state
    {
        parse_mode,
        parse_path_slash,
        parse_path,
        parse_to_newline,
        parse_end,
        parse_error,
    } parser_state = parse_mode;
    int dir_fd = OTHER_FD_START;
    int file_fd = OTHER_FD_START + 1;
    const char *pos = str_start;
    while (parser_state != parse_end && parser_state != parse_error)
    {
        switch (parser_state)
        {
        case parse_mode:
            if (pos == end)
            {
                parser_state = (str_start == pos) ? parse_end : parse_error;
                break;
            }
            if (*pos == ':')
            {
                static const char spawn_str[] = "spawn";
                if (pos - str_start != sizeof(spawn_str) - 1 ||
                    memcmp(spawn_str, str_start, sizeof(spawn_str) - 1) != 0)
                {
                    parser_state = parse_error;
                    break;
                }
                parser_state = parse_path_slash;
                ++pos;
                str_start = pos;
                break;
            }
            ++pos;
            break;
        case parse_path_slash:
            parser_state = parse_path;
            if (pos == end || *pos != '/')
            {
                parser_state = parse_to_newline;
                ++pos;
                break;
            }
            ++pos;
            str_start = pos;
            dir_fd = ROOT_FD;
            break;
        case parse_path:
            if (pos == end || *pos == '\n')
            {
                if (open_at(romfs_server, dir_fd, file_fd, str_start,
                            pos - str_start))
                {
                    struct vfs_stat_result file_stat =
                        stat(romfs_server, file_fd);
                    struct map_result exe_mapping =
                        map(romfs_server, file_fd, 0, file_stat.size);
                    if (!L4_is_nil_fpage(exe_mapping.fpage))
                    {
                        load_executable(exe_mapping.addr);
                    }
                }
                if (dir_fd != ROOT_FD)
                    close(romfs_server, dir_fd);
                close(romfs_server, file_fd);
                dir_fd = ROOT_FD;
                parser_state = parse_mode;
                ++pos;
                str_start = pos;
                break;
            }
            if (*pos == '/')
            {
                if (!open_at(romfs_server, dir_fd, file_fd, str_start,
                             pos - str_start))
                {
                    parser_state = parse_to_newline;
                }
                else
                {
                    if (dir_fd != ROOT_FD)
                        close(romfs_server, dir_fd);
                    swap_fds(&dir_fd, &file_fd);
                }
                ++pos;
                str_start = pos;
            }
            ++pos;
            break;
        case parse_to_newline:
            if (pos == end)
            {
                parser_state = parse_error;
                break;
            }
            if (*pos == '\n')
            {
                str_start = pos + 1;
                parser_state = parse_mode;
            }
            ++pos;
            break;
        case parse_end:
        case parse_error:
            // This should never be reached
            break;
        }
    }
}

void parse_init_config(L4_thread_id romfs_server)
{
    if (!open_root_fd(romfs_server))
        kill_root_thread();
    static const char etc_path[] = "etc";
    if (!open_at(romfs_server, ROOT_FD, ETC_FD, etc_path, sizeof(etc_path) - 1))
        kill_root_thread();
    static const char inittab_path[] = "inittab";
    if (!open_at(romfs_server, ETC_FD, INITTAB_FD, inittab_path,
                 sizeof(inittab_path) - 1))
        kill_root_thread();
    const struct vfs_stat_result inittab_stat = stat(romfs_server, INITTAB_FD);
    if (!inittab_stat.success)
        kill_root_thread();
    const struct map_result inittab_mapping =
        map(romfs_server, INITTAB_FD, 0, inittab_stat.size);
    if (L4_is_nil_fpage(inittab_mapping.fpage))
        kill_root_thread();
    parse(romfs_server, inittab_mapping.addr,
          inittab_mapping.addr + inittab_stat.size);
}
