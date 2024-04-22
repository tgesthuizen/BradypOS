#include "libelf.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

struct mmap_state
{
    int fd;
    int _errno;
};

static int read_elf_file(size_t offset, size_t size, void *data, void *user)
{
    struct mmap_state *state = user;
    if (lseek(state->fd, offset, SEEK_SET) == -1)
    {
        state->_errno = errno;
        errno = 0;
        return -1;
    }
    if (read(state->fd, data, size) != size)
    {
        state->_errno = errno;
        errno = 0;
        return -1;
    }
    return 0;
}

static int libelf_to_mmap_perm(int flags)
{
    int prot = PROT_NONE;
    if (flags & LIBELF_ACCESS_R)
        prot |= PROT_READ;
    if (flags & LIBELF_ACCESS_W)
        prot |= PROT_WRITE;
    if (flags & LIBELF_ACCESS_X)
        prot |= PROT_EXEC;
    return prot;
}

static int mmap_alloc_rw(void **location, size_t size, size_t align, int perm,
                         void *user)
{
    struct mmap_state *state = user;
    *location =
        mmap(NULL, size, libelf_to_mmap_perm(perm), MAP_ANONYMOUS, -1, 0);
    int ret = 0;
    if (*location == MAP_FAILED)
    {
        *location = NULL;
        state->_errno = errno;
        errno = 0;
        ret = 1;
    }
    return ret;
}

static int mmap_map(void **location, size_t offset, size_t size, size_t align,
                    int perm, void *user)
{
    struct mmap_state *state = user;
    *location = mmap(NULL, size, libelf_to_mmap_perm(perm), MAP_SHARED,
                     state->fd, offset);
    int ret = 0;
    if (*location == MAP_FAILED)
    {
        *location = NULL;
        state->_errno = errno;
        errno = 0;
        ret = 1;
    }
    return ret;
}

static struct libelf_ops mmap_ops = {
    .read_elf_file = read_elf_file,
    .alloc_rw = mmap_alloc_rw,
    .map = mmap_map,
    .symbol = NULL,
};

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fputs("Usage: elfdemo-linux <elf-file>", stderr);
        return 1;
    }
    const char *const file_name = argv[1];
    int ret;
    ret = open(file_name, 0, O_RDONLY);
}
