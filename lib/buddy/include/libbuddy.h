#ifndef BRADYPOS_LIBBUDDY_H
#define BRADYPOS_LIBBUDDY_H

#include <stddef.h>

enum libbuddy_error
{
    LIBBUDDY_OK,
    LIBBUDDY_NOMEM,
};

enum
{
    LIBBUDDY_PAGE_SIZE = 512
};

struct libbuddy_allocator
{
    void *(*alloc)(size_t size, void *user);
    void (*free)(void *ptr, void *user);
};

/**
 * A special purpose allocator that will place the metadata for a heap in the
 * heap itself. This requires the memory of the heap to be general-purpose
 * memory. Furthermore, the user context of the allocator has to be set to point
 * to the libbuddy struct of the heap.
 */
extern const struct libbuddy_allocator libbuddy_internal_allocator;

struct libbuddy_state
{
    void *pool_start;
    size_t pool_chunks; // size of the pool / LIBBUDDY_PAGE_SIZE
    void *first_meta_block;

    struct libbuddy_allocator allocator;
    void *user;
    enum libbuddy_error last_error;
};

int libbuddy_init_allocator(struct libbuddy_state *state);
void *libbuddy_alloc(size_t size);
void libbuddy_free(void *ptr);
void libbuddy_free_allocator(struct libbuddy_state *state);

#endif
