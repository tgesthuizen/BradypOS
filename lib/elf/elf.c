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
    if (hdr->e_type != ET_EXEC && hdr->e_type != ET_DYN)
        return LIBELF_INVALID_ELF;
    if (hdr->e_machine != EM_ARM)
        return LIBELF_INVALID_ELF;
    if (hdr->e_version != EV_CURRENT)
        return LIBELF_INVALID_ELF;
    if (hdr->e_phentsize < sizeof(Elf32_Phdr))
        return LIBELF_INVALID_ELF;
    return 0;
}

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

static void *linked_to_loaded_addr(const struct libelf_state *state,
                                   Elf32_Addr addr)
{
    for (size_t i = 0; i < state->num_segments; ++i)
    {
        struct libelf_loaded_segment *const segment = &state->segments[i];
        if (segment->linked_addr <= addr &&
            (segment->linked_addr + segment->size) > addr)
            return (unsigned char *)segment->loaded_addr +
                   (addr - segment->linked_addr);
    }
    return NULL;
}

static int relocate_elf_file(struct libelf_state *state)
{
    // Locate the dynamic tags
    const Elf32_Dyn *current_tag = (const Elf32_Dyn *)state->phdrs;
    // No dynamic tags, no relocations - we are done
    if (!current_tag)
        return LIBELF_OK;

    Elf32_Rel *current_reloc = NULL;
    size_t rel_sz = 0;
    size_t total_rel_sz = 0;
    size_t relcount = 0;
    for (; current_tag->d_tag != DT_NULL; ++current_tag)
    {
        switch (current_tag->d_tag)
        {
        case DT_REL:
            current_reloc =
                linked_to_loaded_addr(state, current_tag->d_un.d_ptr);
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

    (void)rel_sz;
    (void)total_rel_sz;
    (void)relcount;
    // TODO: Respect total_rel_sz, relcount and rel_sz instead of implicit
    // assumptions made below

    for (; current_reloc->r_info != R_ARM_NONE; ++current_reloc)
    {
        switch (current_reloc->r_info)
        {
        case R_ARM_RELATIVE:
        {
            Elf32_Addr *reloc_pos =
                linked_to_loaded_addr(state, current_reloc->r_offset);
            *reloc_pos = (Elf32_Addr)linked_to_loaded_addr(state, *reloc_pos);
        }
        break;
        default:
            // Oops
            ;
            return LIBELF_UNKNOWN_RELOC;
        }
    }
    return LIBELF_OK;
}

// Define memset if not present
__attribute__((weak)) void *memset(void *ptr, int value, size_t size)
{
    unsigned char *cptr = ptr;
    while (size--)
    {
        *cptr++ = value;
    }
    return ptr;
}

int load_elf_file(const struct libelf_ops *ops, struct libelf_state *state,
                  void *user)
{
    Elf32_Ehdr elf_hdr;
    int err;
    // Validate the header
    if (ops->read_elf_file(0, sizeof elf_hdr, (char *)&elf_hdr, user) != 0)
        return LIBELF_IFACE_ERROR;
    err = validate_elf_header(&elf_hdr);
    if (err)
        return err;

    for (int i = 0; i < elf_hdr.e_phnum; ++i)
    {
        Elf32_Phdr phdr;
        ops->read_elf_file(elf_hdr.e_phoff + i * elf_hdr.e_phentsize,
                           sizeof phdr, (char *)&phdr, user);

        switch (phdr.p_type)
        {
        case PT_LOAD:
        {
            struct libelf_loaded_segment *const segment =
                &state->segments[state->num_segments++];
            segment->perm = elf_flags_to_libelf_access(phdr.p_flags);
            segment->linked_addr = phdr.p_vaddr;
            segment->size = phdr.p_memsz;
            void *loc = NULL;
            if (!(phdr.p_flags & PF_W))
            {
                if (ops->map(&loc, phdr.p_offset, phdr.p_filesz, phdr.p_align,
                             phdr.p_vaddr & phdr.p_align, segment->perm,
                             user) != 0)
                {
                    return LIBELF_IFACE_ERROR;
                }
            }
            else
            {
                if (ops->alloc_rw(&loc, phdr.p_memsz, phdr.p_align,
                                  phdr.p_vaddr % phdr.p_align, segment->perm,
                                  user) != 0)
                {
                    return LIBELF_IFACE_ERROR;
                }
                if (ops->read_elf_file(phdr.p_offset, phdr.p_filesz, loc,
                                       user) != 0)
                {
                    return LIBELF_IFACE_ERROR;
                }
                memset((unsigned char *)loc + phdr.p_filesz, 0,
                       phdr.p_memsz - phdr.p_filesz);
            }
            segment->loaded_addr = (uintptr_t)loc;
        }
        break;
        case PT_DYNAMIC:
            state->phdrs =
                (uintptr_t)linked_to_loaded_addr(state, phdr.p_vaddr);
            break;
        }
    }

    relocate_elf_file(state);
    state->entry_point =
        (uintptr_t)linked_to_loaded_addr(state, elf_hdr.e_entry);

    return 0;
}

/**
 * Adapted from System V application binary interface
 * https://archive.org/details/systemvapplicati00unix
 */
static unsigned elf_hash(const unsigned char *name)
{
    unsigned h = 0, g;

    while (*name)
    {
        h = (h << 4) + *name++;
        if (g = (h & 0xf0000000), g)
            h ^= g >> 24;
        h &= ~g;
    }
    return h;
}

__attribute__((weak)) int strcmp(const char *lhs, const char *rhs)
{
    while (*lhs != '\0' && *lhs == *rhs)
    {
        lhs++;
        rhs++;
    }

    return (*(unsigned char *)lhs) - (*(unsigned char *)rhs);
}

int locate_elf_symbol(struct libelf_state *state, const char *symbol,
                      unsigned *value)
{
    if (!state->phdrs)
        return LIBELF_NODYN;
    // Detect locations of data structures in the binary
    Elf32_Dyn *dyn = (void *)state->phdrs;
    char *strtab = NULL;
    const Elf32_Sym *syms = NULL;
    Elf32_Word *hash_map = NULL;
    for (; dyn->d_tag != DT_NULL; ++dyn)
    {
        switch (dyn->d_tag)
        {
        case DT_HASH:
            hash_map = linked_to_loaded_addr(state, dyn->d_un.d_ptr);
            break;
        case DT_SYMTAB:
            syms = linked_to_loaded_addr(state, dyn->d_un.d_ptr);
            break;
        case DT_STRTAB:
            strtab = linked_to_loaded_addr(state, dyn->d_un.d_ptr);
            break;
        }
    }
    if (!strtab || !syms || !hash_map)
        return LIBELF_NODYN;

    // Lookup symbol in hash table
    const Elf32_Word nbucket = hash_map[0];
    Elf32_Word *const buckets = hash_map + 2;
    const Elf32_Word nchain = hash_map[1];
    (void)nchain;
    Elf32_Word *const chains = hash_map + 2 + nbucket;
    const Elf32_Sym *sym = NULL;
    unsigned symidx =
        buckets[elf_hash((const unsigned char *)symbol) % nbucket];
    bool found = false;
    while (1)
    {
        if (symidx == STN_UNDEF)
            break;
        sym = &syms[symidx];
        if (strcmp(&strtab[sym->st_name], symbol) == 0)
        {
            found = true;
            break;
        }
        symidx = chains[symidx];
    }

    if (!found)
    {
        return LIBELF_NOSYM;
    }

    *value = (unsigned)linked_to_loaded_addr(state, sym->st_value);
    return LIBELF_OK;
}
