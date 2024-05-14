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
    extern L4_utcb_t __utcb;
    const L4_msg_tag_t tag = {.raw = msg->raw[0]};
    const unsigned total_words = tag.u + tag.t + 1;
    __L4_mr0 = msg->raw[0];
    if (total_words == 1)
        return;
    __L4_mr1 = msg->raw[1];
    if (total_words == 2)
        return;
    __L4_mr2 = msg->raw[2];
    if (total_words == 3)
        return;
    __L4_mr3 = msg->raw[3];
    if (total_words == 4)
        return;
    __L4_mr4 = msg->raw[4];
    if (total_words == 5)
        return;
    __L4_mr5 = msg->raw[5];
    if (total_words == 6)
        return;
    __L4_mr6 = msg->raw[6];
    if (total_words == 7)
        return;
    __L4_mr7 = msg->raw[7];
    for (unsigned i = 8; i < total_words; ++i)
        __utcb.mr[i - 8] = msg->raw[i];
}

void L4_msg_store(L4_msg_tag_t tag, L4_msg_t *msg)
{
    extern L4_utcb_t __utcb;
    const unsigned total_words = tag.u + tag.t + 1;
    msg->raw[0] = tag.raw;
    if (total_words == 1)
        return;
    msg->raw[1] = __L4_mr1;
    if (total_words == 2)
        return;
    msg->raw[2] = __L4_mr2;
    if (total_words == 3)
        return;
    msg->raw[3] = __L4_mr3;
    if (total_words == 4)
        return;
    msg->raw[4] = __L4_mr4;
    if (total_words == 5)
        return;
    msg->raw[5] = __L4_mr5;
    if (total_words == 6)
        return;
    msg->raw[6] = __L4_mr6;
    if (total_words == 7)
        return;
    msg->raw[7] = __L4_mr7;
    for (unsigned i = 8; i < total_words; ++i)
        msg->raw[i] = __utcb.mr[i - 8];
}

void L4_msg_clear(L4_msg_t *msg);

bool L4_is_map_item(unsigned *m);

