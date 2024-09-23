#include <errno.h>
#include <l4/ipc.h>
#include <l4/utcb.h>
#include <service.h>
#include <string.h>
#include <term.h>

enum
{
    IPC_BUFFER_SIZE = 128,
};

enum pl011_offsets
{
    PL011_UARTDR = 0x000,
    PL011_UARTRSR = 0x004,
    PL011_UARTFR = 0x018,
    PL011_UARTILPR = 0x020,
    PL011_UARTIBRD = 0x024,
    PL011_UARTFBRD = 0x028,
    PL011_UARTLCR_H = 0x02C,
    PL011_UARTCR = 0x030,
    PL011_UARTIFLS = 0x034,
    PL011_UARTIMSC = 0x038,
    PL011_UARTRIS = 0x03C,
    PL011_UARTMIS = 0x040,
    PL011_UARTICR = 0x044,
    PL011_UARTDMACR = 0x048,
    PL011_UARTPeriphID0 = 0xFE0,
    PL011_UARTPeriphID1 = 0xFE4,
    PL011_UARTPeriphID2 = 0xFE8,
    PL011_UARTPeriphID3 = 0xFEC,
    PL011_UARTPCellID0 = 0xFF0,
    PL011_UARTPCellID1 = 0xFF4,
    PL011_UARTPCellID2 = 0xFF8,
    PL011_UARTPCellID3 = 0xFFC,
};

static unsigned char *uart_start;

#define UART_REGISTER(reg, type) (*((volatile type *)uart_start + reg))

extern L4_utcb_t __utcb;
static unsigned char ipc_buffer[IPC_BUFFER_SIZE];

static void make_ipc_error(int errno)
{
    L4_load_mr(
        0, (L4_msg_tag_t){.u = 1, .t = 0, .flags = 0, .label = TERM_ERROR}.raw);
    L4_load_mr(1, errno);
}

static unsigned pl011_handle_read_impl(void *buffer, size_t len)
{
    // TODO: Implement
    (void)buffer;
    (void)len;
    return 0;
}

static void pl011_handle_read(L4_msg_tag_t tag, L4_thread_id from)
{
    (void)from;
    if (tag.u != 2 || tag.t != 0)
    {
        make_ipc_error(EINVAL);
        return;
    }
    unsigned len;
    L4_store_mr(TERM_READ_SIZE, &len);
    const unsigned ret = pl011_handle_read_impl(&ipc_buffer, len);
    const unsigned ret_len = len - ret;

    const struct L4_simple_string_item item = {.c = 0,
                                               .type = L4_data_type_string_item,
                                               .compound = 0,
                                               .length = ret_len,
                                               .ptr = (unsigned)&ipc_buffer};
    L4_load_mr(
        TERM_READ_RET_OP,
        (L4_msg_tag_t){.u = 0, .t = 2, .flags = 0, .label = TERM_READ_RET}.raw);
    L4_load_mrs(TERM_READ_RET_STR, 2, (unsigned *)&item);
}

static unsigned pl011_handle_write_impl(void *buffer, size_t len)
{
    // TODO: Implement
    (void)buffer;
    (void)len;
    return 0;
}

static void pl011_handle_write(L4_msg_tag_t tag, L4_thread_id from)
{
    (void)from;
    if (tag.u != 1 || tag.t != 2)
    {
        make_ipc_error(EINVAL);
        return;
    }
    struct L4_simple_string_item string_item;
    L4_store_mrs(TERM_WRITE_BUF, 2, (unsigned *)&string_item);
    const size_t len =
        pl011_handle_write_impl((void *)string_item.ptr, string_item.length);
    L4_load_mr(
        TERM_WRITE_RET_OP,
        (L4_msg_tag_t){.u = 1, .t = 0, .flags = 0, .label = TERM_WRITE_RET}
            .raw);
    L4_load_mr(TERM_WRITE_RET_SIZE, len);
}

static void register_service()
{
    while (1)
    {
        L4_load_mr(
            SERV_REGISTER_OP,
            (L4_msg_tag_t){.u = 1, .t = 0, .flags = 0, .label = SERV_REGISTER}
                .raw);
        unsigned serv_name;
        L4_thread_id from;
        memcpy(&serv_name, "term", sizeof(unsigned));
        L4_load_mr(SERV_REGISTER_NAME, serv_name);
        const L4_thread_id root_id = L4_global_id(L4_USER_THREAD_START, 1);
        L4_msg_tag_t tag =
            L4_ipc(root_id, root_id, L4_timeouts(L4_never, L4_never), &from);
        if (!L4_ipc_failed(tag) && tag.u == 0 && tag.t == 0 &&
            tag.label == SERV_REGISTER_RET)
        {
            break;
        }
    }
}

int main()
{
    // Detect UART

    // Write UARTLCR_H

    // Write UARTIBRD
    // Write UARTFBRD

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
            pl011_handle_read(tag, from);
            break;
        case TERM_WRITE:
            pl011_handle_write(tag, from);
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

__attribute__((naked)) void _start()
{
    asm("pop {r0, r1}\n\t"
        "movs r9, r1\n\t"
        "movs r2, #0\n\t"
        "movs lr, r2\n\t"
        "bl %c[main]\n\t" ::[main] ""(main));
}
