#include <l4/ipc.h>

bool L4_msg_tag_eq(L4_msg_tag_t lhs, L4_msg_tag_t rhs);
bool L4_msg_tag_ne(L4_msg_tag_t lhs, L4_msg_tag_t rhs);

unsigned L4_untyped_words(L4_msg_tag_t tag);
unsigned L4_typed_words(L4_msg_tag_t tag);
L4_msg_tag_t L4_msg_tag_add_label(L4_msg_tag_t tag, unsigned label);
L4_msg_tag_t *L4_msg_tg_add_label_to(L4_msg_tag_t *tag, unsigned label);
L4_msg_tag_t L4_msg_tag();
void L4_set_msg_tag(L4_msg_tag_t tag);

void L4_put(L4_msg_t *msg, unsigned l, int u, unsigned *ut, unsigned t,
            unsigned *items)
{
    (void)msg;
    (void)l;
    (void)u;
    (void)ut;
    (void)t;
    (void)items;
    // TODO
}
void L4_get(L4_msg_t *msg, unsigned *ut, unsigned *items)
{
    (void)msg;
    (void)ut;
    (void)items;
    // TODO
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
        L4_my_utcb()->mr[i] = msg->raw[i];
}

void L4_msg_store(L4_msg_tag_t tag, L4_msg_t *msg)
{
    const unsigned total_words = tag.u + tag.t + 1;
    for (unsigned i = 0; i < total_words; ++i)
        msg->raw[i] = L4_my_utcb()->mr[i];
}

void L4_msg_clear(L4_msg_t *msg);
void L4_store_mr(int i, unsigned *word);
void L4_load_mr(int i, unsigned word);
void L4_store_mrs(int offset, int count, unsigned *words);
void L4_load_mrs(int offset, int count, const unsigned *words);

struct L4_map_item L4_map_item(L4_fpage_t fpage, unsigned snd_base);
bool L4_is_map_item(unsigned *m);
L4_fpage_t L4_map_item_snd_fpage(struct L4_map_item *item);
unsigned L4_map_item_snd_base(struct L4_map_item *item);

struct L4_grant_item L4_grant_item(L4_fpage_t fpage, unsigned snd_base);
bool L4_is_grant_item(unsigned *m);
L4_fpage_t L4_grant_item_snd_fpage(struct L4_grant_item *item);
unsigned L4_grant_item_snd_base(struct L4_grant_item *item);

bool L4_is_ctrl_xfer_item(unsigned *item);
void L4_ctrl_xfer_item_init(struct L4_ctrl_xfer_item *item, unsigned id);

L4_acceptor_t L4_map_grant_items(L4_fpage_t recv_window);
L4_acceptor_t L4_add_acceptor(L4_acceptor_t lhs, L4_acceptor_t rhs);
void L4_add_acceptor_to(L4_acceptor_t *lhs, L4_acceptor_t rhs);
L4_acceptor_t L4_remove_acceptor(L4_acceptor_t lhs, L4_acceptor_t rhs);
L4_fpage_t L4_recv_window(L4_acceptor_t acceptor);

void L4_accept(L4_acceptor_t acceptor);
void L4_accept_strings(L4_acceptor_t acceptor, struct L4_msg_buffer *strings)
{
    (void)acceptor;
    (void)strings;
    // TODO
}
L4_acceptor_t L4_accepted();
void L4_msg_buffer_clear(L4_msg_buffer_t *buffer)
{
    (void)buffer;
    // TODO
}
void L4_msg_buffer_append_simple_rcv_string(L4_msg_buffer_t *buffer,
                                            unsigned string_item)
{
    (void)buffer;
    (void)string_item;
    // TODO
}
void L4_msg_buffer_append_rcv_string(L4_msg_buffer_t *buffer,
                                     unsigned *string_items)
{
    (void)buffer;
    (void)string_items;
    // TODO
}
void L4_store_br(int i, unsigned *word);
void L4_load_br(int i, unsigned word);
void L4_store_brs(int i, int k, unsigned *words);
void L4_load_brs(int i, int k, unsigned *words);

L4_msg_tag_t L4_ipc(L4_thread_id to, L4_thread_id from_specifier,
                    unsigned timeouts, L4_thread_id *from);

bool L4_ipc_succeeded(L4_msg_tag_t tag);
bool L4_ipc_failed(L4_msg_tag_t tag);
bool L4_ipc_propagated(L4_msg_tag_t tag);
bool L4_ipc_redirected(L4_msg_tag_t tag);
bool L4_ipc_xcpu(L4_msg_tag_t tag);
unsigned L4_error_code();
L4_thread_id L4_intended_receiver();
L4_thread_id L4_actual_sender();
void L4_set_propagation(L4_msg_tag_t *tag);
void L4_virtual_sender(L4_thread_id sender);
unsigned L4_timeouts(L4_time_t snd_timeout, L4_time_t recv_timeout);
L4_time_t L4_send_timeout(unsigned timeout);
L4_time_t L4_recv_timeout(unsigned timeout);
