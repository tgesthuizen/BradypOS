#ifndef BRADYPOS_VFS_H
#define BRADYPOS_VFS_H

enum
{
    /** Maximum length of a path element in the VFS system */
    VFS_PATH_MAX = 64,
};

enum vfs_ops
{
    VFS_OPENROOT,
    VFS_OPENAT,
    VFS_CLOSE,
    VFS_STAT,
    VFS_READ,
    VFS_WRITE,
    VFS_MAP,
};

enum vfs_openroot_args
{
    VFS_OPENROOT_OP,
    VFS_OPENROOT_FD,
};

enum vfs_openat_args
{
    VFS_OPENAT_OP,
    VFS_OPENAT_FD,
    VFS_OPENAT_NEWFD,
    VFS_OPENAT_STR,
};

enum vfs_close_args
{
    VFS_CLOSE_OP,
    VFS_CLOSE_FD,
};

enum vfs_stat_args
{
    VFS_STAT_OP,
    VFS_STAT_FD,
};

enum vfs_read_args
{
    VFS_READ_OP,
    VFS_READ_FD,
    VFS_READ_OFFSET,
    VFS_READ_SIZE,
};

enum vfs_write_args
{
    VFS_WRITE_OP,
    VFS_WRITE_FD,
    VFS_WRITE_OFFSET,
    VFS_WRITE_SIZE,
    VFS_WRITE_DATA,
};

enum vfs_map_args
{
    VFS_MAP_OP,
    VFS_MAP_FD,
    VFS_MAP_OFFSET,
    VFS_MAP_SIZE,
    VFS_MAP_PERM,
};

enum vfs_answer_ops
{
    VFS_ERROR,
    VFS_OPENROOT_RET,
    VFS_OPENAT_RET,
    VFS_CLOSE_RET,
    VFS_STAT_RET,
    VFS_READ_RET,
    VFS_WRITE_RET,
    VFS_MAP_RET,
};

enum vfs_error_ret_args
{
    VFS_ERROR_RET_OP,
    VFS_ERROR_RET_ERRNO,
};

enum vfs_openroot_ret_args
{
    VFS_OPENROOT_RET_OP,
    VFS_OPENROOT_RET_FD,
};

enum vfs_openat_ret_args
{
    VFS_OPENAT_RET_OP,
    VFS_OPENAT_RET_FD,
};

enum vfs_close_ret_args
{
    VFS_CLOSE_RET_OP,
};

enum vfs_file_type
{
    VFS_FT_REGULAR,
    VFS_FT_DIRECTORY,
    VFS_FT_HARDLINK,
    VFS_FT_SYMLINK,
    VFS_FT_OTHER,
};

enum vfs_stat_ret_args
{
    VFS_STAT_RET_OP,
    VFS_STAT_RET_TYPE,
    VFS_STAT_RET_SIZE,
};

enum vfs_read_ret_args
{
    VFS_READ_RET_OP,
    VFS_READ_RET_CONTENT,
};

enum vfs_map_ret_args
{
    VFS_MAP_RET_OP,
    VFS_MAP_RET_ADDR,
    VFS_MAP_RET_MAP_ITEM,
};

#endif
