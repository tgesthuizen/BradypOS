#include <l4/bootinfo.h>

bool L4_boot_info_valid(void *boot_info);
unsigned L4_boot_info_size(void *boot_info);
L4_boot_info_rec_t *L4_boot_info_first_entry(void *boot_info);
unsigned L4_boot_info_entries(void *boot_info);
unsigned L4_boot_rec_type(L4_boot_info_rec_t *rec);
L4_boot_info_rec_t *L4_boot_rec_next(L4_boot_info_rec_t *rec);
