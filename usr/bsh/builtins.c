#include "builtins.h"
#include "fileio.h"
#include "variables.h"
#include "vfs.h"
#include <stddef.h>
#include <vfs/client.h>

int last_exit_code;

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
        static const unsigned char usage[] = "Usage: cd <dir>\n";
        term_write(term_service, usage, sizeof usage);
        // TODO: Print error about usage
        last_exit_code = -1;
        return;
    }
    int ret = (*dir == '/') ? open_recursive(ROOT_FD, dir + 1)
                            : open_recursive(WD_FD, dir);
    if (ret == -1)
    {
        static const unsigned char err_msg[] =
            "Could not open target directory\n";
        term_write(term_service, err_msg, sizeof err_msg);
        last_exit_code = -1;
        return;
    }
    close(romfs_service, WD_FD);
    move_fd(romfs_service, ret, WD_FD);
    last_exit_code = 0;
}

static const struct bsh_builtin cd_builtin = {"cd", handle_cd_builtin};

static void handle_pwd_builtin(char **cmd)
{
    if (cmd[1] != NULL)
    {
        static const unsigned char usage[] = "Usage: pwd\n";
        term_write(term_service, usage, sizeof usage);
        last_exit_code = -1;
        return;
    }
    // TODO: Actually print parent directories
    struct vfs_stat_result stat_res =
        stat(romfs_service, WD_FD, (char *)ipc_buffer);
    if (!stat_res.success)
    {
        static const unsigned char err_msg[] =
            "Internal error: Could not stat current directory\n";
        term_write(term_service, err_msg, sizeof err_msg);
        last_exit_code = -2;
        return;
    }
    term_write(term_service, (unsigned char *)stat_res.file_name,
               stat_res.size);
}

static const struct bsh_builtin pwd_builtin = {"pwd", handle_pwd_builtin};

static void handle_ls_builtin(char **cmd)
{
    // TODO: Handle cmd
    // For now we just print the current directory
    struct vfs_stat_result stat_res =
        stat(romfs_service, WD_FD, (char *)ipc_buffer);
    if (!stat_res.success)
    {
        static const unsigned char err_msg[] =
            "Internal error: Could not stat current directory\n";
        term_write(term_service, err_msg, sizeof err_msg);
        last_exit_code = -1;
        return;
    }
    // Consistency check: PWD is a directory
    if (stat_res.type != VFS_FT_DIRECTORY)
    {
        static const unsigned char err_msg[] =
            "Internal error: Current directory is not a directory\n";
        term_write(term_service, err_msg, sizeof err_msg);
        last_exit_code = -1;
        return;
    }
    for (struct dirent_t dirent =
             readdir(romfs_service, WD_FD, 0, (char *)ipc_buffer);
         dirent.success;
         dirent = readdir(romfs_service, WD_FD, dirent.next_offset,
                          (char *)ipc_buffer))
    {
        dirent.file_name[dirent.size] = '\n';
        dirent.file_name[++dirent.size] = '\0';
        term_write(term_service, (unsigned char *)dirent.file_name,
                   dirent.size);
    }
    last_exit_code = 0;
}

static const struct bsh_builtin ls_builtin = {"ls", handle_ls_builtin};

const struct bsh_builtin *builtins[] = {&cd_builtin, &pwd_builtin, &ls_builtin,
                                        NULL};
