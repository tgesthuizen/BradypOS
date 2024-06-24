#include <errno.h>
#include <l4/ipc.h>
#include <l4/schedule.h>
#include <l4/thread.h>
#include <l4/utcb.h>
#include <semihosting.h>
#include <service.h>
#include <string.h>
#include <term.h>

enum
{
    SHTERM_STACK_SIZE = 512,
    IPC_BUFFER_SIZE = 128,
};

enum fd_numbers
{
    STDIN,
    STDOUT,
    STDERR,
};

register unsigned char *got_location asm("r9");

__attribute__((naked)) void _start()
{
    asm("pop {r0, r1}\n\t"
        "movs r9, r1\n\t"
        "movs r2, #0\n\t"
        "movs lr, r2\n\t"
        "bl main\n\t");
}

extern L4_utcb_t __utcb;

static unsigned char *ipc_buffer[IPC_BUFFER_SIZE];

static void make_ipc_error(int errno)
{
    L4_load_mr(
        0, (L4_msg_tag_t){.u = 1, .t = 0, .flags = 0, .label = TERM_ERROR}.raw);
    L4_load_mr(1, errno);
}

static void shterm_handle_read(L4_msg_tag_t tag, L4_thread_id from)
{
    (void)from;
    if (tag.u != 2 || tag.t != 0)
    {
        make_ipc_error(EINVAL);
        return;
    }
    unsigned len;
    L4_store_mr(TERM_READ_SIZE, &len);
    len = sh_read(STDIN, &ipc_buffer, len);
    const struct L4_simple_string_item item = {.c = 0,
                                               .type = L4_data_type_string_item,
                                               .compound = 0,
                                               .length = len,
                                               .ptr = (unsigned)&ipc_buffer};
    L4_load_mr(
        TERM_READ_RET_OP,
        (L4_msg_tag_t){.u = 0, .t = 2, .flags = 0, .label = TERM_READ_RET}.raw);
    L4_load_mrs(TERM_READ_RET_STR, 2, (unsigned *)&item);
}

static void shterm_handle_write(L4_msg_tag_t tag, L4_thread_id from)
{
    (void)from;
    if (tag.u != 2 || tag.t != 2)
    {
        make_ipc_error(EINVAL);
        return;
    }
    struct L4_simple_string_item string_item;
    L4_store_mrs(TERM_WRITE_BUF, 2, (unsigned *)&string_item);
    const size_t len =
        sh_write(STDOUT, (void *)string_item.ptr, string_item.length);
    L4_load_mr(
        TERM_WRITE_RET_OP,
        (L4_msg_tag_t){.u = 1, .t = 0, .flags = 0, .label = TERM_WRITE_RET}
            .raw);
    L4_load_mr(TERM_WRITE_RET_SIZE, len);
}

static void register_service()
{
    L4_load_mr(
        SERV_REGISTER_OP,
        (L4_msg_tag_t){.u = 1, .t = 0, .flags = 0, .label = SERV_REGISTER}.raw);
    unsigned serv_name;
    L4_thread_id from;
    memcpy(&serv_name, "serv", sizeof(unsigned));
    L4_load_mr(SERV_REGISTER_NAME, serv_name);
    L4_ipc(L4_global_id(L4_USER_THREAD_START, 1), L4_NILTHREAD,
           L4_timeouts(L4_never, L4_never), &from);
}

int main()
{
    register_service();
    const L4_acceptor_t acceptor = L4_string_items_acceptor;
    const struct L4_simple_string_item string_acceptor = {
        .c = 0,
        .type = L4_data_type_string_item,
        .compound = 0,
        .length = IPC_BUFFER_SIZE,
        .ptr = (unsigned)&ipc_buffer};

    while (1)
    {
        L4_load_br(0, acceptor.raw);
        L4_load_brs(1, 2, (unsigned *)&string_acceptor);
        L4_thread_id from;
        L4_msg_tag_t tag = L4_ipc(L4_NILTHREAD, L4_ANYTHREAD,
                                  L4_timeouts(L4_never, L4_never), &from);
        if (L4_ipc_failed(tag))
        {
            continue;
        }
        switch (tag.label)
        {
        case TERM_READ:
            shterm_handle_read(tag, from);
            break;
        case TERM_WRITE:
            shterm_handle_write(tag, from);
            break;
        default:
            make_ipc_error(EINVAL);
            break;
        }
        tag =
            L4_ipc(from, L4_NILTHREAD, L4_timeouts(L4_never, L4_never), &from);
        if (L4_ipc_failed(tag))
        {
            continue;
        }
    }
}
