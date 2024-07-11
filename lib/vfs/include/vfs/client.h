#ifndef BRADYPOS_VFS_CLIENT_H
#define BRADYPOS_VFS_CLIENT_H

#include <l4/space.h>
#include <l4/thread.h>
#include <stdbool.h>
#include <stddef.h>
#include <vfs.h>

bool open_root_fd(L4_thread_id romfs_server, int fd);
bool open_at(L4_thread_id romfs_server, int fd, int new_fd, const char *path,
             size_t pathlen);
bool close(L4_thread_id romfs_server, int fd);

struct vfs_stat_result
{
    bool success;
    enum vfs_file_type type;
    size_t size;
};

struct vfs_stat_result stat(L4_thread_id romfs_server, int fd);

struct map_result
{
    L4_fpage_t fpage;
    void *addr;
};

struct map_result map(L4_thread_id romfs_server, int fd, size_t offset,
                      size_t size);

#endif
