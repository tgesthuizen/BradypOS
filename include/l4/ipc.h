#ifndef BRADYPOS_L4_IPC_H
#define BRADYPOS_L4_IPC_H

#include <l4/fpage.h>
#include <l4/syscalls.h>
#include <l4/time.h>
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

enum L4_msg_tag_flags
{
    L4_msg_tag_flag_propagated_ipc,
    L4_msg_tag_flag_redirected_ipc,
    L4_msg_tag_flag_cross_processor_ipc,
    L4_msg_tag_flag_error_indicator,
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

extern volatile L4_utcb_t __utcb;

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
    unsigned raw[L4_MR_COUNT];
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

union L4_acceptor
{
    struct
    {
        unsigned s : 1;
        unsigned c : 1;
        unsigned reserved : 2;
        unsigned recv_window : 28;
    };
    unsigned raw;
};

typedef union L4_acceptor L4_acceptor_t;

#define L4_untyped_words_acceptor                                              \
    ((L4_acceptor_t){.s = 0, .c = 0, .recv_window = L4_NILPAGE.raw})
#define L4_string_items_acceptor                                               \
    ((L4_acceptor_t){.s = 1, .c = 0, .recv_window = L4_NILPAGE.raw})
#define L4_ctrl_xfer_items_acceptor                                            \
    ((L4_acceptor_t){.s = 0, .c = 1, .recv_window = L4_NILPAGE.raw})

inline L4_fpage_t L4_recv_window(L4_acceptor_t acceptor);

inline L4_acceptor_t L4_add_acceptor(L4_acceptor_t lhs, L4_acceptor_t rhs)
{
    L4_acceptor_t res = {.raw = 0};
    res.s = lhs.s || rhs.s;
    res.c = lhs.c || rhs.c;
    const L4_fpage_t rhs_recv_page = L4_recv_window(rhs);
    res.recv_window =
        rhs_recv_page.raw == 0 ? lhs.recv_window : rhs.recv_window;
    return res;
}
inline void L4_add_acceptor_to(L4_acceptor_t *lhs, L4_acceptor_t rhs)
{
    *lhs = L4_add_acceptor(*lhs, rhs);
}

inline L4_acceptor_t L4_remove_acceptor(L4_acceptor_t lhs, L4_acceptor_t rhs)
{
    L4_acceptor_t res;
    res.s = rhs.s ? 0 : lhs.s;
    res.c = rhs.c ? 0 : rhs.c;
    res.recv_window = rhs.recv_window ? 0 : lhs.recv_window;
    return res;
}
inline void L4_remove_acceptor_from(L4_acceptor_t *lhs, L4_acceptor_t rhs)
{
    *lhs = L4_remove_acceptor(*lhs, rhs);
}

inline bool L4_has_string_items(L4_acceptor_t acceptor) { return acceptor.s; }
inline bool L4_has_map_grant_items(L4_acceptor_t acceptor)
{
    return L4_recv_window(acceptor).raw != L4_NILTHREAD;
}

inline L4_fpage_t L4_recv_window(L4_acceptor_t acceptor)
{
    return (L4_fpage_t){.perm = 0,
                        .s = acceptor.recv_window & ((1 << 7) - 1),
                        .b = acceptor.recv_window >> 6};
}

struct L4_msg_buffer;

inline void L4_accept(L4_acceptor_t acceptor) { __utcb.br[0] = acceptor.raw; }
void L4_accept_strings(L4_acceptor_t acceptor, struct L4_msg_buffer *strings);
inline L4_acceptor_t L4_accepted()
{
    return (L4_acceptor_t){.raw = __utcb.br[0]};
}

struct L4_msg_buffer
{
    unsigned raw[L4_BR_COUNT];
};
typedef struct L4_msg_buffer L4_msg_buffer_t;

void L4_msg_buffer_clear(L4_msg_buffer_t *buffer);
void L4_msg_buffer_append_simple_rcv_string(L4_msg_buffer_t *buffer,
                                            unsigned string_item);
void L4_msg_buffer_append_rcv_string(L4_msg_buffer_t *bufer,
                                     unsigned *string_items);

inline void L4_store_br(int i, unsigned *word) { *word = __utcb.br[i]; }
inline void L4_load_br(int i, unsigned word) { __utcb.br[i] = word; }
inline void L4_store_brs(int i, int k, unsigned *words)
{
    for (int j = 0; j < k; ++j)
        words[j] = __utcb.br[i + j];
}
inline void L4_load_brs(int i, int k, unsigned *words)
{
    for (int j = 0; j < k; ++j)
        __utcb.br[i + j] = words[j];
}

// Non-standard extension
enum L4_ipc_flag_bits
{
    L4_ipc_flag_propagated_ipc,
    L4_ipc_flag_redirected_ipc,
    L4_ipc_flag_cross_processor_ipc,
    L4_ipc_flag_error_indicator,
};

// Non-standard extension
enum L4_ipc_error_code
{
    L4_ipc_error_none,
    L4_ipc_error_timeout,
    L4_ipc_error_nonexistent_partner,
    L4_ipc_error_canceled,
    L4_ipc_error_message_overflow,
    L4_ipc_error_xfer_timeout_invoker,
    L4_ipc_error_xfer_timeout_partner,
    L4_ipc_error_aborted,
};

// Non-standard extension
union L4_ipc_error
{
    struct
    {
        unsigned p : 1;
        unsigned err : 3;
        unsigned offset : 28;
    };
    unsigned raw;
};
typedef union L4_ipc_error L4_ipc_error_t;

inline L4_msg_tag_t L4_ipc(L4_thread_id to, L4_thread_id from_specifier,
                           unsigned timeouts, L4_thread_id *from)
{
    register L4_thread_id r0 asm("r0") = to;
    register L4_thread_id r1 asm("r1") = from_specifier;
    register unsigned r2 asm("r2") = timeouts;
    L4_thread_id result;
    asm volatile("movs r7, %[SYS_IPC]\n\t"
                 "svc #0\n\t"
                 : "=l"(result)
                 : [SYS_IPC] "i"(SYS_IPC), "0"(r0), "l"(r1), "l"(r2)
                 : "r7", "memory");
    *from = result;
    return L4_msg_tag();
}

inline bool L4_ipc_succeeded(L4_msg_tag_t tag)
{
    return (tag.flags & L4_ipc_flag_error_indicator) == 0;
}
inline bool L4_ipc_failed(L4_msg_tag_t tag)
{
    return (tag.flags & L4_ipc_flag_error_indicator) != 0;
}
inline bool L4_ipc_propagated(L4_msg_tag_t tag)
{
    return (tag.flags & L4_ipc_flag_propagated_ipc) != 0;
}
inline bool L4_ipc_redirected(L4_msg_tag_t tag)
{
    return (tag.flags & L4_ipc_flag_redirected_ipc) != 0;
}
inline bool L4_ipc_xcpu(L4_msg_tag_t tag)
{
    return (tag.flags & L4_ipc_flag_cross_processor_ipc) != 0;
}
inline unsigned L4_error_code() { return __utcb.error; }
inline L4_thread_id L4_intended_receiver() { return __utcb.intended_receiver; }
inline L4_thread_id L4_actual_sender() { return __utcb.sender; }
inline void L4_set_propagation(L4_msg_tag_t *tag)
{
    tag->flags |= L4_ipc_flag_propagated_ipc;
}
inline void L4_virtual_sender(L4_thread_id sender) { __utcb.sender = sender; }
inline unsigned L4_timeouts(L4_time_t snd_timeout, L4_time_t recv_timeout)
{
    return ((unsigned)snd_timeout.raw << 16) | recv_timeout.raw;
}

#endif
