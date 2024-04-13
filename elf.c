#include "elf.h"
#include <libelf.h>

#include <stdbool.h>

enum
{
    LIBELF_MAX_SEGMENTS = 8
};

static int validate_elf_header(const Elf32_Ehdr *hdr)
{
    if (hdr->e_ident[EI_MAG0] != ELFMAG0 || hdr->e_ident[EI_MAG1] != ELFMAG1 ||
        hdr->e_ident[EI_MAG2] != ELFMAG2 || hdr->e_ident[EI_MAG3] != ELFMAG3)
        return LIBELF_INVALID_ELF;
    if (hdr->e_ident[EI_CLASS] != ELFCLASS32)
        return LIBELF_INVALID_ELF;
    if (hdr->e_ident[EI_OSABI] != ELFOSABI_ARM_AEABI)
        return LIBELF_INVALID_ELF;
    if (hdr->e_ident[EI_DATA] != ELFDATA2LSB)
        return LIBELF_INVALID_ELF;
    if (hdr->e_type != ET_EXEC)
        return LIBELF_INVALID_ELF;
    if (hdr->e_machine != EM_ARM)
        return LIBELF_INVALID_ELF;
    if (hdr->e_version != EV_CURRENT)
        return LIBELF_INVALID_ELF;
    if (hdr->e_phentsize < sizeof(Elf32_Phdr))
        return LIBELF_INVALID_ELF;
    if (hdr)
        return 0;
}

struct libelf_loaded_segment
{
    Elf32_Phdr phdr;
    unsigned loaded_addr;
    bool read_only;
};

static enum libelf_access_permission elf_flags_to_libelf_access(int flags)
{
    int res = 0;
    if (flags & PF_R)
        res |= LIBELF_ACCESS_R;
    if (flags & PF_W)
        res |= LIBELF_ACCESS_W;
    if (flags & PF_X)
        res |= LIBELF_ACCESS_X;
    return (enum libelf_access_permission)res;
}

int load_elf_file(const struct libelf_ops *ops, int flags, void *user)
{
    Elf32_Ehdr elf_hdr;
    int err;
    // Validate the header
    if (!ops->read_elf_file(0, sizeof elf_hdr, (char *)&elf_hdr, user))
        return LIBELF_IFACE_ERROR;
    err = validate_elf_header(&elf_hdr);
    if (err)
        return err;

    // Load all segments
    int segment_idx = 0;
    struct libelf_loaded_segment segments[LIBELF_MAX_SEGMENTS];
    for (int i = 0; i < elf_hdr.e_phnum; ++i)
    {
        Elf32_Phdr phdr;
        ops->read_elf_file(elf_hdr.e_phoff + i * elf_hdr.e_phentsize,
                           sizeof phdr, (char *)&phdr, user);
        if (phdr.p_type != PT_LOAD)
            continue;
        segments[segment_idx].phdr = phdr;
        if (phdr.p_flags & PF_W)
        {
            segments[segment_idx].read_only = false;
            void *loc = NULL;
            if (phdr.p_flags & LIBELF_LOAD_STATIC)
            {
                loc = (void *)phdr.p_paddr;
            }
            if (!ops->map_segment(&loc, phdr.p_memsz, phdr.p_align,
                                  LIBELF_ACCESS_RWX, user))
                return LIBELF_IFACE_ERROR;
            if (!ops->read_elf_file(phdr.p_offset, phdr.p_filesz, loc, user))
                return LIBELF_IFACE_ERROR;
        }
        else
        {
            // Map segment from ROM
            segments[segment_idx].read_only = true;
            void *loc = NULL;
            if (!ops->map_segment(&loc, phdr.p_filesz, phdr.p_align,
                                  LIBELF_ACCESS_R, user))
                return LIBELF_IFACE_ERROR;
        }
        ++segment_idx;
    }
    return 0;
}
