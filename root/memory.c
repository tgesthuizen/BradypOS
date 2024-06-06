#include "memory.h"
#include "l4/ipc.h"
#include <errno.h>
#include <linked_list_alloc.h>
#include <root.h>

// The RAM segment of the executable ends after BSS
extern unsigned char __bss_end;

enum
{
    RPI_PICO_SRAM_BASE = 0x20000000,
    RPI_PICO_SRAM_SIZE = 0x00042000,
    RPI_PICO_SRAM_END = RPI_PICO_SRAM_BASE + RPI_PICO_SRAM_SIZE,
};

static struct linked_list_alloc_state state;

void init_memory_management()
{
    unsigned char *heap_base = &__bss_end;
    // Align to next page
    heap_base =
        (unsigned char *)(((unsigned)heap_base +
                           (LINKED_LIST_ALLOC_PAGE_SIZE - 1)) &
                          ((1 << (LINKED_LIST_ALLOC_PAGE_SIZE_LOG2 + 1)) - 1));
    const size_t num_pages = (RPI_PICO_SRAM_END - (unsigned)heap_base) >>
                             LINKED_LIST_ALLOC_PAGE_SIZE_LOG2;
    init_linked_list_alloc_state(&state, heap_base, num_pages);
}

void *alloc_pages(size_t num_pages)
{
    return linked_list_alloc_pages(&state, num_pages);
}
void free_pages(void *base, size_t num_pages)
{
    return linked_list_free_pages(&state, base, num_pages);
}

static void create_ipc_alloc_error(int errno)
{
    L4_msg_tag_t tag;
    tag.flags = 0;
    tag.label = ROOT_ERROR;
    tag.u = 1;
    tag.t = 0;
    L4_load_mr(0, tag.raw);
    L4_load_mr(1, errno);
}

/**
 * TODO: Right now we only provide information to the calling process which
 * memory region is free for it.
 * Update the protocol so that actual map objects are handed out and make sure
 * to also take access rights away when a process frees pages.
 */
void handle_ipc_alloc(L4_msg_tag_t msg_tag)
{
    if (msg_tag.u != 1 || msg_tag.t != 0)
    {
        create_ipc_alloc_error(EINVAL);
        return;
    }
    unsigned num_pages;
    L4_store_mr(1, &num_pages);
    void *mem = alloc_pages(num_pages);
    if (mem == NULL)
    {
        create_ipc_alloc_error(ENOMEM);
        return;
    }
    L4_msg_tag_t answer_tag;
    answer_tag.flags = 0;
    answer_tag.label = ROOT_ALLOC_MEM_RET;
    answer_tag.u = 2;
    answer_tag.t = 0;
    L4_load_mr(0, answer_tag.raw);
    L4_load_mr(1, (unsigned)mem);
    L4_load_mr(2, num_pages);
}

void handle_ipc_free(L4_msg_tag_t msg_tag)
{
    if (msg_tag.u != 2 || msg_tag.t != 0)
    {
        create_ipc_alloc_error(EINVAL);
        return;
    }
    unsigned mem;
    unsigned num_pages;
    L4_store_mr(1, &mem);
    L4_store_mr(2, &num_pages);
    free_pages((void *)mem, num_pages);
    L4_msg_tag_t answer_tag;
    answer_tag.flags = 0;
    answer_tag.label = ROOT_FREE_MEM_RET;
    answer_tag.u = 0;
    answer_tag.t = 0;
    L4_load_mr(0, answer_tag.raw);
}
