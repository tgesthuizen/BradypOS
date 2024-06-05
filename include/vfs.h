#ifndef BRADYPOS_VFS_H
#define BRADYPOS_VFS_H

enum
{
    /** Maximum length of a path element in the VFS system */
    VFS_PATH_MAX = 64,
};

enum vfs_ops
{
    VFS_OPENAT,
    VFS_CLOSE,
    VFS_READ,
    VFS_WRITE,
};

enum vfs_openat_args
{
    VFS_OPENAT_OP,
    VFS_OPENAT_FD,
    VFS_OPENAT_STR,
};

enum vfs_close_args
{
    VFS_CLOSE_OP,
    VFS_CLOSE_FD,
};

enum vfs_read_args
{
    VFS_READ_OP,
    VFS_READ_FD,
    VFS_READ_OFFSET,
    VFS_READ_SIZE,
};

#endif
