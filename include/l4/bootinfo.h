#ifndef BRADYPOS_L4_BOOTINFO_H
#define BRADYPOS_L4_BOOTINFO_H

#include <stdbool.h>

enum
{
    L4_BOOT_INFO_MAGIC = 0x14B0021D,
};

enum L4_boot_info_record_type
{
    L4_BOOT_INFO_MODULE = 0x1,
    L4_BOOT_INFO_EXE = 0x2,
    L4_BOOT_INFO_VARIABLES = 0x40,
    L4_BOOT_INFO_EFI_TABLES = 0x101,
    L4_BOOT_INFO_MULTIBOOT = 0x102,
};

struct L4_boot_info_header
{
    unsigned magic;
    unsigned version;
    unsigned size;
    unsigned first_entry;
    unsigned num_entries;
};

struct L4_boot_info_rec
{
    unsigned type;
    unsigned version;
    unsigned offset_next;
};

typedef struct L4_boot_info_rec L4_boot_info_rec_t;

struct L4_boot_segment_info
{
    unsigned pstart;
    unsigned vstart;
    unsigned size;
};

struct L4_boot_info_record_module
{
    unsigned type;
    unsigned version;
    unsigned offset_next;
    unsigned start;
    unsigned size;
    unsigned cmdline_offset;
};

struct L4_boot_info_record_exe
{
    unsigned type;
    unsigned version;
    unsigned offset_next;
    struct L4_boot_segment_info text;
    struct L4_boot_segment_info data;
    struct L4_boot_segment_info bss;
    unsigned initial_ip;
    unsigned flags;

    unsigned label;
    unsigned cmdline_offset;
};

enum
{
    L4_BOOT_INFO_VARIABLE_NAME_LEN = 12,
};

struct L4_boot_info_variables_entry
{
    char name[L4_BOOT_INFO_VARIABLE_NAME_LEN];
    unsigned value;
};

struct L4_boot_info_variables
{
    unsigned type;
    unsigned version;
    unsigned offset_next;
    unsigned variable_count;
    struct L4_boot_info_variables_entry entries[];
};

inline bool L4_boot_info_valid(void *boot_info)
{
    struct L4_boot_info_header *const bi = boot_info;
    return bi->magic == L4_BOOT_INFO_MAGIC;
}

inline unsigned L4_boot_info_size(void *boot_info)
{
    struct L4_boot_info_header *const bi = boot_info;
    return bi->size;
}

inline L4_boot_info_rec_t *L4_boot_info_first_entry(void *boot_info)
{
    struct L4_boot_info_header *const bi = boot_info;
    return (L4_boot_info_rec_t *)((unsigned char *)bi + bi->first_entry);
}

inline unsigned L4_boot_info_entries(void *boot_info)
{
    struct L4_boot_info_header *const bi = boot_info;
    return bi->num_entries;
}

inline unsigned L4_boot_rec_type(L4_boot_info_rec_t *rec) { return rec->type; }
inline L4_boot_info_rec_t *L4_boot_rec_next(L4_boot_info_rec_t *rec)
{
    return (L4_boot_info_rec_t *)((unsigned char *)rec + rec->offset_next);
}

#endif
