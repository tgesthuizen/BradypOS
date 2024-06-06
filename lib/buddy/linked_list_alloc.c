#include <linked_list_alloc.h>

struct linked_list_node
{
    size_t num_pages;
    struct linked_list_node *next;
    struct linked_list_node *prev;
};

void init_linked_list_alloc_state(struct linked_list_alloc_state *state,
                                  void *base, size_t num_pages)
{
    state->start = base;
    state->page_count = num_pages;
    state->first_free_item = base;
    *(struct linked_list_node *)state->first_free_item =
        (struct linked_list_node){
            .num_pages = num_pages, .prev = NULL, .next = NULL};
}

static void split_node_at(struct linked_list_node *node, size_t split_at)
{
    const size_t new_size = node->num_pages - split_at;
    struct linked_list_node *new_node =
        (struct linked_list_node *)((unsigned char *)node + 512 * split_at);
    new_node->prev = node;
    new_node->next = node->next;
    new_node->num_pages = new_size;
    node->next = new_node;
}

static void delete_node_from_list(struct linked_list_node **node)
{
    struct linked_list_node *const prev = (*node)->prev;
    struct linked_list_node *const next = (*node)->next;
    if (prev)
        prev->next = next;
    if (next)
        next->prev = prev;
    // Required if we're editing the pointer into the list
    *node = (*node)->next;
}

void *linked_list_alloc_pages(struct linked_list_alloc_state *state,
                              size_t num_pages)
{
    struct linked_list_node **current_node =
        (struct linked_list_node **)&state->first_free_item;
    while (*current_node && (*current_node)->num_pages < num_pages)
        current_node = &(*current_node)->next;
    if (*current_node == NULL)
    {
        // We did not find enough free memory to satisfy the alloc
        return NULL;
    }
    if ((*current_node)->num_pages > num_pages)
    {
        split_node_at(*current_node, num_pages);
    }
    void *const base = *current_node;
    delete_node_from_list(current_node);
    return base;
}

void linked_list_free_pages(struct linked_list_alloc_state *state, void *base,
                            size_t num_pages)
{
    struct linked_list_node *node = base;
    struct linked_list_node **first_node_ptr =
        (struct linked_list_node **)&state->first_free_item;
    *node = (struct linked_list_node){
        .num_pages = num_pages, .prev = NULL, .next = *first_node_ptr};
    *first_node_ptr = node;
}
