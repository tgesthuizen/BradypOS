#ifndef BRADYPOS_VFS_H
#define BRADYPOS_VFS_H

enum
{
    /** Maximum length of a path element in the VFS system */
    VFS_PATH_MAX = 64,
};

// TODO: Add operation for mapping files to memory
enum vfs_ops
{
    VFS_OPENROOT,
    VFS_OPENAT,
    VFS_CLOSE,
    VFS_READ,
    VFS_WRITE,
};

enum vfs_openroot_args
{
    VFS_OPENROOT_OP,
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

enum vfs_answer_ops
{
    VFS_ERROR,
    VFS_OPENROOT_RET,
    VFS_OPENAT_RET,
    VFS_CLOSE_RET,
    VFS_READ_RET,
    VFS_WRITE_RET,
};

#endif
