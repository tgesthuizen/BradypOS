#include "bootinfo.h"
#include "libelf.h"
#include "string.h"

enum boot_info_record_type
{
    BOOT_INFO_EXE = 0x2,
};

struct boot_info_header
{
    unsigned magic;
    unsigned version;
    unsigned size;
    unsigned first_entry;
    unsigned num_entries;
};

struct boot_segment_info
{
    unsigned pstart;
    unsigned vstart;
    unsigned size;
};

static void elf_to_boot_segment(const struct libelf_loaded_segment *in,
                                struct boot_segment_info *out)
{
    out->pstart = in->loaded_addr;
    out->vstart = in->loaded_addr;
    out->size = in->size;
}

struct boot_info_record_exe
{
    unsigned type;
    unsigned version;
    unsigned offset_next;
    struct boot_segment_info text;
    struct boot_segment_info data;
    struct boot_segment_info bss;
    unsigned initial_ip;
    unsigned flags;
    unsigned label;
    unsigned cmdline_offset;
};

struct full_boot_info
{
    struct boot_info_header hdr;
    struct boot_info_record_exe kern;
    struct boot_info_record_exe root;
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
    info->hdr.first_entry = sizeof(struct boot_info_header);
    info->hdr.num_entries = 2;
    info->hdr.size = sizeof(struct full_boot_info);
    info->kern.type = BOOT_INFO_EXE;
    info->kern.version = 1;
    info->kern.offset_next = sizeof(struct boot_info_record_exe);
    elf_to_boot_segment(&kern_state->segments[0], &info->kern.text);
    elf_to_boot_segment(&kern_state->segments[1], &info->kern.data);
    info->kern.bss =
        (struct boot_segment_info){.pstart = 0, .vstart = 0, .size = 0};
    info->kern.flags = 0;
    memcpy(&info->kern.label, "kern", sizeof(unsigned));
    info->kern.cmdline_offset = 0;

    info->root.type = BOOT_INFO_EXE;
    info->root.version = 1;
    info->root.offset_next = sizeof(struct boot_info_record_exe);
    elf_to_boot_segment(&root_state->segments[0], &info->root.text);
    elf_to_boot_segment(&root_state->segments[1], &info->root.data);
    info->root.bss =
        (struct boot_segment_info){.pstart = 0, .vstart = 0, .size = 0};
    info->root.flags = 0;
    memcpy(&info->root.label, "root", sizeof(unsigned));
    info->root.cmdline_offset = 0;
}
