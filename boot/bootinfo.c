#include "string.h"
#include <l4/bootinfo.h>
#include <libelf.h>

static void elf_to_boot_segment(const struct libelf_loaded_segment *in,
                                struct L4_boot_segment_info *out)
{
    out->pstart = in->loaded_addr;
    out->vstart = in->loaded_addr;
    out->size = in->size;
}

struct full_boot_info
{
    struct L4_boot_info_header hdr;
    struct L4_boot_info_record_exe kern;
    struct L4_boot_info_record_exe root;
};

enum
{
    BOOT_INFO_MAGIC = 0x14B0021D
};

void construct_boot_info(struct libelf_state *kern_state,
                         struct libelf_state *root_state, void *addr)
{
    struct full_boot_info *info = addr;
    info->hdr.magic = BOOT_INFO_MAGIC;
    info->hdr.version = 1;
    info->hdr.first_entry = sizeof(struct L4_boot_info_header);
    info->hdr.num_entries = 2;
    info->hdr.size = sizeof(struct full_boot_info);
    info->kern.type = L4_BOOT_INFO_EXE;
    info->kern.version = 1;
    info->kern.offset_next = sizeof(struct L4_boot_info_record_exe);
    elf_to_boot_segment(&kern_state->segments[0], &info->kern.text);
    elf_to_boot_segment(&kern_state->segments[1], &info->kern.data);
    info->kern.bss =
        (struct L4_boot_segment_info){.pstart = 0, .vstart = 0, .size = 0};
    info->kern.initial_ip = kern_state->entry_point;
    info->kern.flags = 0;
    memcpy(&info->kern.label, "kern", sizeof(unsigned));
    info->kern.cmdline_offset = 0;

    info->root.type = L4_BOOT_INFO_EXE;
    info->root.version = 1;
    info->root.offset_next = sizeof(struct L4_boot_info_record_exe);
    elf_to_boot_segment(&root_state->segments[0], &info->root.text);
    elf_to_boot_segment(&root_state->segments[1], &info->root.data);
    info->root.bss =
        (struct L4_boot_segment_info){.pstart = 0, .vstart = 0, .size = 0};
    info->root.initial_ip = root_state->entry_point;
    info->root.flags = 0;
    memcpy(&info->root.label, "root", sizeof(unsigned));
    info->root.cmdline_offset = 0;
}
