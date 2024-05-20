#include <l4/bootinfo.h>
#include <libelf.h>
#include <string.h>

static void elf_to_boot_segment(const struct libelf_loaded_segment *in,
                                struct L4_boot_segment_info *out)
{
    out->pstart = in->loaded_addr;
    out->vstart = in->loaded_addr;
    out->size = in->size;
}

enum
{
    BOOTINFO_VARIABLE_COUNT = 3
};

struct full_boot_info
{
    struct L4_boot_info_header hdr;
    struct L4_boot_info_record_exe kern;
    struct L4_boot_info_record_exe root;
    struct L4_boot_info_variables vars;
    struct L4_boot_info_variables_entry var_entries[BOOTINFO_VARIABLE_COUNT];
};

enum
{
    BOOT_INFO_MAGIC = 0x14B0021D
};

#define COPY_LITERAL(s, x) memcpy((s), x, sizeof(x));

void construct_boot_info(struct libelf_state *kern_state,
                         struct libelf_state *root_state, void *addr)
{
    struct full_boot_info *info = addr;
    info->hdr.magic = BOOT_INFO_MAGIC;
    info->hdr.version = 1;
    info->hdr.first_entry = sizeof(struct L4_boot_info_header);
    info->hdr.num_entries = 3;
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

    info->vars.type = L4_BOOT_INFO_VARIABLES;
    info->vars.version = 1;
    info->vars.offset_next = 0;
    info->vars.variable_count = BOOTINFO_VARIABLE_COUNT;

    COPY_LITERAL(info->var_entries[0].name, "root_got");
    locate_elf_symbol(root_state, "__global_offset_table",
                      &info->var_entries[0].value);
    COPY_LITERAL(info->var_entries[1].name, "root_utcb");
    locate_elf_symbol(root_state, "__utcb", &info->var_entries[1].value);
    extern unsigned char __romfs_start[];
    COPY_LITERAL(info->var_entries[2].name, "rootfs_base");
    info->var_entries[2].value = (unsigned)&__romfs_start;
}
