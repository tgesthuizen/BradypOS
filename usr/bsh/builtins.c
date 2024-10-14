#include "builtins.h"
#include "fileio.h"
#include "variables.h"
#include <stddef.h>
#include <vfs/client.h>

static int open_recursive(int root_fd, const char *absolute_path)
{
    int cur_fd = root_fd;
    int next_fd = TEMP1_FD;

    const char *ptr = absolute_path;
    const char *prev_ptr = absolute_path;
    while (1)
    {
        for (; *ptr && *ptr != '/'; ++ptr)
            ;
        if (!open_at(romfs_service, cur_fd, next_fd, prev_ptr, ptr - prev_ptr))
            return -1;
        {
            const int tmp = cur_fd;
            cur_fd = next_fd;
            next_fd = tmp;
        }
        // Sort out next_fd if neccessary
        if (next_fd == root_fd)
            next_fd = TEMP2_FD;
        else
            close(romfs_service, next_fd);

        while (*ptr == '/')
            ++ptr;
        if (*ptr == '\0')
            return cur_fd;
    }
}

static void handle_cd_builtin(char **cmd)
{
    ++cmd;
    const char *const dir = *cmd++;
    if (*cmd)
    {
        // TODO: Print error about usage
        return;
    }
    int ret = (*dir == '/') ? open_recursive(ROOT_FD, dir + 1)
                            : open_recursive(WD_FD, dir);
    if (ret == -1)
    {
        return;
    }
    close(romfs_service, WD_FD);
    move_fd(romfs_service, ret, WD_FD);
}

static const struct bsh_builtin cd_builtin = {"cd", handle_cd_builtin};

extern struct bsh_builtin pwd_builtin;
extern struct bsh_builtin ls_builtin;

const struct bsh_builtin *builtins[] = {&cd_builtin, &pwd_builtin, &ls_builtin,
                                        NULL};
