#ifndef BRADYPOS_ROOT_MEMORY_H
#define BRADYPOS_ROOT_MEMORY_H

#include <l4/ipc.h>
#include <stddef.h>

void init_memory_management();
void *alloc_pages(size_t num_pages);
void free_pages(void *base, size_t num_pages);

void handle_ipc_alloc(L4_msg_tag_t msg_tag);
void handle_ipc_free(L4_msg_tag_t msg_tag);

#endif
