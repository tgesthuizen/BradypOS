#ifndef BRADYPOS_L4_IPC_H
#define BRADYPOS_L4_IPC_H

#include <l4/utcb.h>
#include <stdbool.h>

// L4 5.1 Messages and Message Registers

union L4_msg_tag
{
    struct
    {
        unsigned u : 6;
        unsigned t : 6;
        unsigned flags : 4;
        unsigned label : 16;
    };
    unsigned raw;
};

typedef union L4_msg_tag L4_msg_tag_t;

#define L4_NILTAG ((L4_msg_tag_t){.raw = 0})

inline bool L4_is_msg_tag_eq(L4_msg_tag_t lhs, L4_msg_tag_t rhs)
{
    return lhs.raw == rhs.raw;
}

inline bool L4_is_msg_tag_ne(L4_msg_tag_t lhs, L4_msg_tag_t rhs)
{
    return lhs.raw != rhs.raw;
}

inline unsigned L4_label(L4_msg_tag_t tag) { return tag.label; }
inline unsigned L4_untyped_words(L4_msg_tag_t tag) { return tag.u; }
inline unsigned L4_typed_words(L4_msg_tag_t tag) { return tag.t; }
inline L4_msg_tag_t L4_msg_tag_add_label(L4_msg_tag_t tag, unsigned label)
{
    tag.label = label;
    return tag;
}
inline L4_msg_tag_t *L4_msg_tg_add_label_to(L4_msg_tag_t *tag, unsigned label)
{
    tag->label = label;
    return tag;
}

extern L4_utcb_t __utcb;

inline L4_msg_tag_t L4_msg_tag()
{
    L4_msg_tag_t res;
    res.raw = __utcb.mr[0];
    return res;
}
inline void L4_set_msg_tag(L4_msg_tag_t tag)
{
    __utcb.mr[0] = (unsigned)tag.raw;
}

struct L4_msg
{
    unsigned raw[16];
};

typedef struct L4_msg L4_msg_t;

void L4_put(L4_msg_t *msg, unsigned l, int u, unsigned *ut, unsigned t,
            unsigned *items);
void L4_get(L4_msg_t *msg, unsigned *ut, unsigned *items);
inline L4_msg_tag_t L4_msg_msg_tag(L4_msg_t *msg)
{
    L4_msg_tag_t res;
    res.raw = msg->raw[0];
    return res;
}
inline void L4_set_msg_msg_tag(L4_msg_t *msg, L4_msg_tag_t tag)
{
    msg->raw[0] = tag.raw;
}

inline unsigned L4_msg_label(L4_msg_t *msg)
{
    return L4_msg_msg_tag(msg).label;
}
inline void L4_set_msg_label(L4_msg_t *msg, unsigned label)
{
    L4_msg_tag_t *tag = (L4_msg_tag_t *)&msg->raw[0];
    tag->label = label;
}

void L4_msg_load(L4_msg_t *msg);
void L4_msg_store(L4_msg_tag_t tag, L4_msg_t *msg);

inline void L4_msg_clear(L4_msg_t *msg)
{
    L4_msg_tag_t tag;
    tag.raw = msg->raw[0];
    tag.u = 0;
    tag.t = 0;
    msg->raw[0] = tag.raw;
}

// TODO: Append, put and get functions

// Low-Level MR Access

inline void L4_store_mr(int i, unsigned *word) { *word = __utcb.mr[i]; }
inline void L4_load_mr(int i, unsigned word) { __utcb.mr[i] = word; }
inline void L4_store_mrs(int offset, int count, unsigned *words)
{
    for (int i = 0; i < count; ++i)
        words[i] = __utcb.mr[i + offset];
}
inline void L4_load_mrs(int offset, int count, const unsigned *words)
{
    for (int i = 0; i < count; ++i)
        __utcb.mr[i + offset] = words[i];
}

enum L4_typed_item_kind
{
    L4_data_type_map_item = 0b100,
    L4_data_type_grant_item = 0b101,
    L4_data_type_ctrl_xfer_item = 0b110,
    L4_data_type_string_item = 0b000,
};

struct L4_map_item
{
    unsigned c : 1;
    unsigned type : 3;
    unsigned reserved : 6;
    unsigned snd_base : 22;
    unsigned perm : 4;
    unsigned snd_fpage : 28;
};

inline bool L4_is_map_item(unsigned *m)
{
    return ((struct L4_map_item *)m)->type == L4_data_type_map_item;
}

struct L4_grant_item
{
    unsigned c : 1;
    unsigned type : 3;
    unsigned reserved : 6;
    unsigned snd_base : 22;
    unsigned perm : 4;
    unsigned snd_fpage : 28;
};

struct L4_ctrl_xfer_item
{
    unsigned c : 1;
    unsigned type : 3;
    unsigned id : 8;
    unsigned mask : 20;
    unsigned psr;
    unsigned r[16];
};

struct L4_simple_string_item
{
    unsigned c : 1;
    unsigned type : 3;
    unsigned reserved : 5;
    unsigned compound : 1;
    unsigned length : 22;
    unsigned ptr;
};

struct L4_compound_string_item
{
    unsigned c : 1;
    unsigned type : 3;
    unsigned j : 5;
    unsigned cont : 1;
    unsigned substring_length : 22;
};

#endif
