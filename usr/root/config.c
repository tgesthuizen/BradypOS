#include "config.h"
#include "exec.h"
#include <l4/ipc.h>
#include <l4/schedule.h>
#include <l4/space.h>
#include <string.h>
#include <vfs/client.h>

enum
{
    IPC_BUFFER_SIZE = 128,
};

void kill_root_thread();

enum
{
    ROOT_FD = 3,
    ETC_FD = 4,
    INITTAB_FD = 5,
    OTHER_FD_START,
};

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
                dir_fd = OTHER_FD_START;
                file_fd = OTHER_FD_START + 1;
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
    if (!open_root_fd(romfs_server, ROOT_FD))
        kill_root_thread();
    static const char etc_path[] = "etc";
    if (!open_at(romfs_server, ROOT_FD, ETC_FD, etc_path, sizeof(etc_path) - 1))
        kill_root_thread();
    static const char inittab_path[] = "inittab";
    if (!open_at(romfs_server, ETC_FD, INITTAB_FD, inittab_path,
                 sizeof(inittab_path) - 1))
        kill_root_thread();
    close(romfs_server, ETC_FD);
    const struct vfs_stat_result inittab_stat = stat(romfs_server, INITTAB_FD);
    if (!inittab_stat.success)
        kill_root_thread();
    const struct map_result inittab_mapping =
        map(romfs_server, INITTAB_FD, 0, inittab_stat.size);
    if (L4_is_nil_fpage(inittab_mapping.fpage))
        kill_root_thread();
    parse(romfs_server, inittab_mapping.addr,
          inittab_mapping.addr + inittab_stat.size);
    close(romfs_server, INITTAB_FD);
}
