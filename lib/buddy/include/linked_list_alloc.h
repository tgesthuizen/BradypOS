#ifndef BRADYPOS_BUDDY_LINKED_LIST_ALLOC_H
#define BRADYPOS_BUDDY_LINKED_LIST_ALLOC_H

#include <stddef.h>

enum
{

    LINKED_LIST_ALLOC_PAGE_SIZE_LOG2 = 9,
    LINKED_LIST_ALLOC_PAGE_SIZE = 1 << LINKED_LIST_ALLOC_PAGE_SIZE_LOG2,
};

struct linked_list_alloc_state
{
    void *start;
    size_t page_count;
    void *first_free_item;
};

void init_linked_list_alloc_state(struct linked_list_alloc_state *state,
                                  void *start, size_t page_count);
void *linked_list_alloc_pages(struct linked_list_alloc_state *state,
                              size_t num_pages);
void linked_list_free_pages(struct linked_list_alloc_state *state, void *base,
                            size_t num_pages);

#endif
