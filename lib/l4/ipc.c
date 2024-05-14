#include <l4/ipc.h>

bool L4_msg_tag_eq(L4_msg_tag_t lhs, L4_msg_tag_t rhs);
bool L4_msg_tag_ne(L4_msg_tag_t lhs, L4_msg_tag_t rhs);
unsigned L4_label(L4_msg_tag_t tag);
unsigned L4_untyped_words(L4_msg_tag_t tag);
unsigned L4_typed_words(L4_msg_tag_t tag);
L4_msg_tag_t L4_msg_tag_add_label(L4_msg_tag_t tag, unsigned label);
L4_msg_tag_t *L4_msg_tg_add_label_to(L4_msg_tag_t *tag, unsigned label);
L4_msg_tag_t L4_msg_tag();
void L4_set_msg_tag(L4_msg_tag_t tag);

void L4_put(L4_msg_t *msg, unsigned l, int u, unsigned *ut, unsigned t,
            unsigned *items)
{ // TODO
}
void L4_get(L4_msg_t *msg, unsigned *ut, unsigned *items)
{ // TODO
}

L4_msg_tag_t L4_msg_msg_tag(L4_msg_t *msg);
void L4_set_msg_msg_tag(L4_msg_t *msg, L4_msg_tag_t tag);
unsigned L4_msg_label(L4_msg_t *msg);
void L4_set_msg_label(L4_msg_t *msg, unsigned label);

void L4_msg_load(L4_msg_t *msg)
{
    const L4_msg_tag_t tag = {.raw = msg->raw[0]};
    const unsigned total_words = tag.u + tag.t + 1;
    for (unsigned i = 0; i < total_words; ++i)
        __utcb.mr[i] = msg->raw[i];
}

void L4_msg_store(L4_msg_tag_t tag, L4_msg_t *msg)
{
    const unsigned total_words = tag.u + tag.t + 1;
    for (unsigned i = 0; i < total_words; ++i)
        msg->raw[i] = __utcb.mr[i];
}

void L4_msg_clear(L4_msg_t *msg);
void L4_store_mr(int i, unsigned *word);
void L4_load_mr(int i, unsigned word);

bool L4_is_map_item(unsigned *m);
