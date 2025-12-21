#include "memory.h"
#include "variables.h"
#include <errno.h>
#include <l4/bootinfo.h>
#include <l4/fpage.h>
#include <l4/ipc.h>
#include <l4/kip.h>
#include <l4/space.h>
#include <linked_list_alloc.h>
#include <root.h>
#include <stddef.h>

// The RAM segment of the executable ends after BSS
extern unsigned char __bss_end;

enum
{
    RPI_PICO_SRAM_BASE = 0x20000000,
    RPI_PICO_SRAM_SIZE = 0x00042000,
    RPI_PICO_SRAM_END = RPI_PICO_SRAM_BASE + RPI_PICO_SRAM_SIZE,
};

static struct linked_list_alloc_state state;

static unsigned align_to_page(unsigned value)
{
    const unsigned align_mask = LINKED_LIST_ALLOC_PAGE_SIZE - 1;
    return (value + align_mask) & ~align_mask;
}

static unsigned find_heap_start()
{
    struct L4_boot_info_header *const header =
        L4_read_kip_ptr(the_kip, the_kip->boot_info);
    const unsigned num_entries = L4_boot_info_entries(header);
    L4_boot_info_rec_t *current_entry = L4_boot_info_first_entry(header);
    unsigned highest_bss_address = 0;
    for (unsigned i = 0; i < num_entries;
         ++i, current_entry = L4_boot_rec_next(current_entry))
    {
        if (L4_boot_rec_type(current_entry) != L4_BOOT_INFO_EXE)
        {
            continue;
        }
        struct L4_boot_info_record_exe *const exe =
            (struct L4_boot_info_record_exe *)current_entry;
        const unsigned max_address_data = exe->data.vstart + exe->data.size;
        if (max_address_data > highest_bss_address)
            highest_bss_address = max_address_data;
        const unsigned max_address_bss = exe->bss.vstart + exe->bss.size;
        if (max_address_bss > highest_bss_address)
            highest_bss_address = max_address_bss;
    }
    return align_to_page(highest_bss_address);
}

void init_memory_management()
{
    const unsigned heap_base_aligned = find_heap_start();
    const unsigned num_pages = (RPI_PICO_SRAM_END - heap_base_aligned) >>
                               LINKED_LIST_ALLOC_PAGE_SIZE_LOG2;
    init_linked_list_alloc_state(&state, (unsigned char *)heap_base_aligned,
                                 num_pages);
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
    if (msg_tag.u != 2 || msg_tag.t != 0)
    {
        create_ipc_alloc_error(EINVAL);
        return;
    }
    L4_fpage_t requested_fpage;
    L4_store_mr(1, &requested_fpage.raw);
    void *mem = alloc_pages(1 << (requested_fpage.s + 9));
    if (mem == NULL)
    {
        create_ipc_alloc_error(ENOMEM);
        return;
    }
    L4_fpage_t allocated_fpage =
        L4_fpage_add_rights(L4_fpage_log2((unsigned)mem, requested_fpage.s + 9),
                            L4_rights(requested_fpage));
    struct L4_grant_item grant_item = L4_grant_item(allocated_fpage, 0);
    L4_msg_tag_t answer_tag;
    answer_tag.flags = 0;
    answer_tag.label = ROOT_ALLOC_MEM_RET;
    answer_tag.u = 1;
    answer_tag.t = 2;
    L4_load_mr(0, answer_tag.raw);
    L4_load_mrs(1, 2, (unsigned *)&grant_item);
}

void handle_ipc_free(L4_msg_tag_t msg_tag)
{
    if (msg_tag.u != 2 || msg_tag.t != 0)
    {
        create_ipc_alloc_error(EINVAL);
        return;
    }
    L4_fpage_t target_page;
    L4_store_mr(1, &target_page.raw);
    L4_load_mr(0, target_page.raw);
    L4_unmap((L4_unmap_control_t){.f = 1, .k = 0, .reserved = 0}.raw);
    free_pages((void *)L4_address(target_page),
               L4_size(target_page) / (1 << 9));
    L4_msg_tag_t answer_tag;
    answer_tag.flags = 0;
    answer_tag.label = ROOT_FREE_MEM_RET;
    answer_tag.u = 0;
    answer_tag.t = 0;
    L4_load_mr(0, answer_tag.raw);
}
