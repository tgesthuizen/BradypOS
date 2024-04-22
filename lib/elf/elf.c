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
    // if (hdr->e_ident[EI_OSABI] != ELFOSABI_ARM_AEABI)
    //     return LIBELF_INVALID_ELF;
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
    return 0;
}

struct libelf_loaded_segment
{
    Elf32_Phdr phdr;
    void *loaded_addr;
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

static bool phdr_contains(const Elf32_Phdr *phdr, Elf32_Addr addr)
{
    return phdr->p_vaddr <= addr && phdr->p_vaddr + phdr->p_memsz > addr;
}

static void *linked_to_loaded_addr(const struct libelf_loaded_segment *segment,
                                   Elf32_Addr addr)
{
    return ((unsigned char *)segment->loaded_addr) +
           (addr - segment->phdr.p_vaddr);
}

int load_elf_file(const struct libelf_ops *ops, int flags, void *user)
{
    Elf32_Ehdr elf_hdr;
    int err;
    // Validate the header
    if (ops->read_elf_file(0, sizeof elf_hdr, (char *)&elf_hdr, user) != 0)
        return LIBELF_IFACE_ERROR;
    err = validate_elf_header(&elf_hdr);
    if (err)
        return err;

    // Load all segments
    Elf32_Phdr dynamic_header;
    int segment_idx = 0;
    struct libelf_loaded_segment segments[LIBELF_MAX_SEGMENTS];
    for (int i = 0; i < elf_hdr.e_phnum; ++i)
    {
        Elf32_Phdr phdr;
        ops->read_elf_file(elf_hdr.e_phoff + i * elf_hdr.e_phentsize,
                           sizeof phdr, (char *)&phdr, user);
        // Stash away dynamic phdr if we encounter it
        if (phdr.p_type == PT_DYNAMIC)
        {
            dynamic_header = phdr;
            continue;
        }
        if (phdr.p_type != PT_LOAD)
        {
            continue;
        }
        segments[segment_idx].phdr = phdr;
        segments[segment_idx].read_only = !(phdr.p_flags & PF_W);
        if (segments[segment_idx].read_only)
        {
            void *loc = NULL;
            if (ops->map(&loc, phdr.p_offset, phdr.p_filesz, phdr.p_align,
                         elf_flags_to_libelf_access(phdr.p_flags), user) != 0)
            {
                return LIBELF_IFACE_ERROR;
            }
            segments[segment_idx].loaded_addr = loc;
        }
        else
        {
            void *loc;
            if (ops->alloc_rw(&loc, phdr.p_memsz, phdr.p_align,
                              elf_flags_to_libelf_access(phdr.p_flags),
                              user) != 0)
            {
                return LIBELF_IFACE_ERROR;
            }
            segments[segment_idx].loaded_addr = loc;
            if (ops->read_elf_file(phdr.p_offset, phdr.p_filesz, loc, user) !=
                0)
            {
                return LIBELF_IFACE_ERROR;
            }
            for (size_t i = phdr.p_filesz; i < phdr.p_memsz; ++i)
            {
                ((unsigned char *)loc)[i] = 0;
            }
        }
        ++segment_idx;
    }

    // Locate the dynamic tags
    const Elf32_Dyn *current_tag = NULL;
    for (int i = 0; i < segment_idx; ++i)
    {
        if (phdr_contains(&segments[i].phdr, dynamic_header.p_vaddr))
        {
            current_tag =
                linked_to_loaded_addr(&segments[i], dynamic_header.p_vaddr);
            break;
        }
    }

    Elf32_Rel *current_reloc = NULL;
    size_t rel_sz = 0;
    size_t total_rel_sz = 0;
    size_t relcount = 0;
    for (; current_tag->d_tag != DT_NULL; ++current_tag)
    {
        switch (current_tag->d_tag)
        {
        case DT_REL:
        {
            // Lookup loaded address by linked address
            Elf32_Addr linked_addr = current_tag->d_un.d_ptr;
            for (int i = 0; i < segment_idx; ++i)
            {
                if (phdr_contains(&segments[i].phdr, linked_addr))
                {
                    current_reloc =
                        linked_to_loaded_addr(&segments[i], linked_addr);
                    break;
                }
            }
        }
        break;
        case DT_RELCOUNT:
            relcount = current_tag->d_un.d_val;
            break;
        case DT_RELSZ:
            total_rel_sz = current_tag->d_un.d_val;
            break;
        case DT_RELENT:
            rel_sz = current_tag->d_un.d_val;
            break;
        }
    }

    for (; current_reloc->r_info != R_ARM_NONE; ++current_reloc)
    {
        switch (current_reloc->r_info)
        {
        case R_ARM_RELATIVE:
        {
            Elf32_Addr *reloc_pos = NULL;
            for (int i = 0; i < segment_idx; ++i)
            {
                if (phdr_contains(&segments[i].phdr, current_reloc->r_offset))
                {
                    reloc_pos = linked_to_loaded_addr(&segments[i],
                                                      current_reloc->r_offset);
                    break;
                }
            }
            for (int i = 0; i < segment_idx; ++i)
            {
                if (phdr_contains(&segments[i].phdr, *reloc_pos))
                {
                    *reloc_pos = (Elf32_Addr)linked_to_loaded_addr(&segments[i],
                                                                   *reloc_pos);
                    break;
                }
            }
        }
        break;
        default:
            // Oops
            ;
            return LIBELF_UNKNOWN_RELOC;
        }
    }

    return 0;
}
