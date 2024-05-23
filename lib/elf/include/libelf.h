#ifndef BRADYPOS_LIBELF_H
#define BRADYPOS_LIBELF_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

enum libelf_access_permission
{
    LIBELF_ACCESS_R = 1,
    LIBELF_ACCESS_W = 2,
    LIBELF_ACCESS_X = 4,
};

struct libelf_ops
{
    int (*read_elf_file)(size_t offset, size_t size, void *data, void *user);
    /**
     * ELF has an interesting idea of alignment: Allocations have to respect the
     * alignment, but the source section can not be aligned to the alignment
     * requirement, and the same offset must be added to the actually linked
     * address. I.e. align = 2^9, vaddr=515, then a valid loaded address would
     * be 1027, but 1024 wouldn't be.
     */
    int (*alloc_rw)(void **location, size_t size, size_t align,
                    size_t align_offset, int perm, void *user);
    int (*map)(void **location, size_t offset, size_t size, size_t align,
               size_t align_offset, int perm, void *user);
    int (*symbol)(char *name, size_t *loc, void *user);
};

enum libelf_error
{
    LIBELF_OK,
    LIBELF_IFACE_ERROR,
    LIBELF_INVALID_ELF,
    LIBELF_UNKNOWN_RELOC,
    LIBELF_NODYN,
    LIBELF_NOSYM,
};

struct libelf_loaded_segment
{
    uintptr_t linked_addr;
    uintptr_t loaded_addr;
    size_t size;
    int perm;
};

struct libelf_state
{
    uintptr_t entry_point;
    uintptr_t phdrs;
    size_t num_segments;
    size_t max_segments;
    struct libelf_loaded_segment *segments;
};

int load_elf_file(const struct libelf_ops *ops, struct libelf_state *state,
                  void *user);
int locate_elf_symbol(struct libelf_state *state, const char *symbol,
                      unsigned *value);

#ifdef __cplusplus
}
#endif

#endif
