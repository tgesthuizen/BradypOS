#include <libbuddy.h>
#include <stdbool.h>

struct libbuddy_block_info
{
    struct libbuddy_block_info *prev;
    struct libbuddy_block_info *next;
    void *where;
    size_t log_size;
    bool allocated;
};

struct libbuddy_meta_block
{
    struct libbuddy_meta_block *next_meta_block;
    size_t slots_used;
};

enum
{
    LIBBUDDY_SLOTS_PER_PAGE =
        (LIBBUDDY_PAGE_SIZE - sizeof(struct libbuddy_meta_block)) /
        sizeof(struct libbuddy_block_info)
};

struct libbuddy_ctrl_block
{
    struct libbuddy_meta_block meta;
    struct libbuddy_block_info info[LIBBUDDY_SLOTS_PER_PAGE];
};

static unsigned ceillog2(unsigned value)
{
    unsigned log = sizeof(value) - __builtin_clz(value);
    return log + (log != value);
}

static struct libbuddy_ctrl_block *create_new_control_block(void *where,
                                                            size_t order)
{
    struct libbuddy_ctrl_block *b = where;
    b->meta.slots_used = order;
    b->info[0] = (struct libbuddy_block_info){.next = &b->info[1],
                                              .prev = 0,
                                              .where = &b->info[0],
                                              .log_size = 0,
                                              .allocated = true};
    unsigned char *addr = b->info[0].where + (1 << 9);
    for (unsigned idx = 1; idx < order; ++idx)
    {
        b->info[idx] = (struct libbuddy_block_info){
            .next = &b->info[idx + 1],
            .prev = &b->info[idx - 1],
            .where = addr,
            .log_size = idx - 1,
            .allocated = false,
        };
        addr += 1 << (9 + idx - 1);
    }
    return b;
}

static bool internal_alloc_setup_heap(struct libbuddy_state *alloc)
{
    if (alloc->pool_chunks == 0)
    {
        alloc->last_error = LIBBUDDY_NOMEM;
        return false;
    }

    const unsigned max_block_order = ceillog2(alloc->pool_chunks);
    struct libbuddy_ctrl_block *b =
        create_new_control_block(alloc->pool_start, max_block_order);
    alloc->first_meta_block = &b->meta;

    return true;
}

static void *interal_alloc(size_t size, void *user)
{
    struct libbuddy_state *alloc = user;
    struct libbuddy_meta_block *root_block = alloc->user;
    if (!root_block)
        internal_alloc_setup_heap(alloc);
    // Step 1: Find the optimal free block
    const size_t target_size = ceillog2(size);
}

int libbuddy_init_allocator(struct libbuddy_state *state) {}
