#include "exec.h"
#include "memory.h"
#include "variables.h"
#include <l4/ipc.h>
#include <l4/space.h>
#include <l4/thread.h>
#include <l4/time.h>
#include <l4/utcb.h>
#include <libelf.h>
#include <string.h>

struct root_elf_load_state
{
    unsigned char *elf_base;
    unsigned map_count;
};

static int elf_read_file(size_t offset, size_t size, void *data, void *user)
{
    struct root_elf_load_state *const state = user;
    memcpy(data, state->elf_base + offset, size);
    return 0;
}

static int elf_to_l4_perm(int perm)
{
    return ((perm & LIBELF_ACCESS_R) != 0) * L4_readable |
           ((perm & LIBELF_ACCESS_W) != 0) * L4_writable |
           ((perm & LIBELF_ACCESS_X) != 0) * L4_executable;
}

static int add_mapping(struct root_elf_load_state *state,
                       struct L4_map_item item)
{
    if (1 + state->map_count * 2 + 2 >= L4_MR_COUNT)
    {
        return 1;
    }
    item.c = 1;
    memcpy(&__utcb.mr[1 + state->map_count * 2], &item, sizeof(unsigned) * 2);
    ++state->map_count;
    return 0;
}

static int elf_alloc_rw(void **location, size_t size, size_t align,
                        size_t align_offset, int perm, void *user)
{
    (void)align;
    (void)align_offset;
    (void)perm;
    struct root_elf_load_state *state = user;
    void *const addr = alloc_pages(size / 512);
    if (addr == NULL)
        return 1;
    const struct L4_map_item map_item =
        L4_map_item(L4_fpage_add_rights(L4_fpage((unsigned)addr, size),
                                        elf_to_l4_perm(perm)),
                    (unsigned)addr);
    if (add_mapping(state, map_item) != 0)
        return 1;
    // TODO: Compensate for possible align_offset
    *location = addr;
    return 0;
}

static int elf_map(void **location, size_t offset, size_t size, size_t align,
                   size_t align_offset, int perm, void *user)
{
    (void)align;
    (void)align_offset;
    struct root_elf_load_state *const state = user;
    void *const addr = state->elf_base + offset;
    const struct L4_map_item map_item =
        L4_map_item(L4_fpage_add_rights(L4_fpage((unsigned)addr, size),
                                        elf_to_l4_perm(perm)),
                    (unsigned)addr);
    if (add_mapping(state, map_item) != 0)
        return 1;
    *location = addr;
    return 0;
}

static const struct libelf_ops elf_ops = {
    .read_elf_file = elf_read_file,
    .alloc_rw = elf_alloc_rw,
    .map = elf_map,
    .symbol = NULL,
};

bool load_executable(unsigned char *elf_base)
{
    // Load ELF file
    enum
    {
        MAX_LOADED_SEGMENTS = 4
    };
    static struct libelf_loaded_segment segments[MAX_LOADED_SEGMENTS];
    static struct libelf_state state;
    state = (struct libelf_state){.max_segments = MAX_LOADED_SEGMENTS,
                                  .num_segments = 0,
                                  .segments = segments};

    struct root_elf_load_state root_state = {.elf_base = elf_base,
                                             .map_count = 0};
    if (load_elf_file(&elf_ops, &state, &root_state) != 0)
    {
        return false;
    }
    // Get required symbols
    unsigned utcb;
    unsigned sp;
    if (locate_elf_symbol(&state, "__utcb", &utcb) != 0)
    {
        return false;
    }
    if (locate_elf_symbol(&state, "__sp", &sp) != 0)
    {
        return false;
    }
    // Create thread
    const L4_thread_id program_id = L4_global_id(next_thread_no++, 1);
    if (L4_thread_control(program_id, program_id, my_thread_id, my_thread_id,
                          (void *)utcb) == 0)
    {
        return false;
    }
    unsigned old_ctrl;
    if (L4_space_control(program_id, 0,
                         L4_fpage_add_rights(
                             L4_fpage_log2((unsigned)the_kip, 10), L4_readable),
                         L4_fpage_add_rights(L4_fpage_log2(utcb, 9),
                                             L4_readable | L4_writable),
                         L4_NILTHREAD, &old_ctrl) == 0)
    {
        return false;
    }

    // Map segments of loaded ELF files into created thread
    struct L4_map_item *item_ptr =
        ((struct L4_map_item *)&__utcb.mr[1 + root_state.map_count * 2]);
    struct L4_map_item item = *item_ptr;
    item.c = 0;
    *item_ptr = item;
    __utcb.mr[0] =
        (L4_msg_tag_t){
            .u = 0, .t = root_state.map_count * 2, .flags = 0, .label = 1}
            .raw;
    L4_thread_id from;
    L4_msg_tag_t answer_tag =
        L4_ipc(program_id, L4_NILTHREAD,
               L4_timeouts(L4_zero_time, L4_zero_time), &from);
    if (L4_ipc_failed(answer_tag))
    {
        return false;
    }
    // Pager start protocol, start the thread
    __utcb.mr[0] = (L4_msg_tag_t){.u = 2, .t = 0, .flags = 0, .label = 1}.raw;
    __utcb.mr[1] = state.entry_point;
    __utcb.mr[2] = sp;
    answer_tag = L4_ipc(program_id, L4_NILTHREAD,
                        L4_timeouts(L4_zero_time, L4_zero_time), &from);
    if (L4_ipc_failed(answer_tag))
    {
        return false;
    }
    return true;
}
