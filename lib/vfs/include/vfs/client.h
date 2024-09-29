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
    char *file_name;
};

/**
 * @brief Returns the stats for @p fd in @p romfs_server
 * @param romfs_server Thread ID of the target server
 * @param fd File descriptor to get stats for
 * @param result Struct to store results in
 *
 * @p result needs to have file_name point to a buffer of at least VFS_PATH_MAX
 * bytes.
 */
struct vfs_stat_result stat(L4_thread_id romfs_server, int fd,
                            char *filename_buf);

struct map_result
{
    L4_fpage_t fpage;
    void *addr;
};

struct map_result map(L4_thread_id romfs_server, int fd, size_t offset,
                      size_t size);

struct dirent_t
{
    enum vfs_file_type type;
    size_t size;
    size_t next_offset;
    char *file_name;
    bool success;
};

/**
 * @brief Reads a directory entry from @p fd in @p romfs_server
 * @param romfs_server Thread ID of the server to contact.
 * @param fd File descriptor of the directory in question
 * @param offset Offset of the target file in the directory
 * @param filename_buf Buffer of at least @c VFS_PATH_MAX bytes
 */
struct dirent_t readdir(L4_thread_id romfs_server, int fd, size_t offset,
                        char *filename_buf);

#endif
