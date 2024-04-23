#ifndef BRADYPOS_BOOT_BOOTINFO_H
#define BRADYPOS_BOOT_BOOTINFO_H

#include <libelf.h>

void construct_boot_info(struct libelf_state *kern_state,
                         struct libelf_state *root_state, void *addr);

#endif
