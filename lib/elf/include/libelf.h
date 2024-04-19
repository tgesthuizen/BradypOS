#ifndef BRADYPOS_LIBELF_H
#define BRADYPOS_LIBELF_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

enum libelf_access_permission
{
    LIBELF_ACCESS_R = 1,
    LIBELF_ACCESS_W = 2,
    LIBELF_ACCESS_X = 4,
    LIBELF_ACCESS_RWX = LIBELF_ACCESS_R | LIBELF_ACCESS_W | LIBELF_ACCESS_X,
};

struct libelf_ops
{
    int (*read_elf_file)(size_t offset, size_t size, void *data, void *user);
    int (*map_segment)(void **location, size_t size, size_t align, int perm,
                       void *user);
    int (*remap_segment)(void *location, size_t size, int perm, void *user);
    int (*symbol)(char *name, size_t *loc, void *user);
};

enum libelf_load_flags
{
    LIBELF_LOAD_STATIC = 1,
    LIBELF_FROM_ROM = 2,
};

enum libelf_error
{
    LIBELF_IFACE_ERROR,
    LIBELF_INVALID_ELF,
    LIBELF_CANNOT_LOAD_INPLACE,
};

int load_elf_file(const struct libelf_ops *ops, int flags, void *user);

#ifdef __cplusplus
}
#endif

#endif
