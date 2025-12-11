/* adapted pl011-term driver
 *
 * - hardware control separated from L4 event loop
 * - non-blocking read/write semantics:
 *     * read_impl: copies up to 'len' bytes from internal RX ring buffer
 * (filled from HW)
 *     * write_impl: writes as many bytes as UART TX FIFO accepts immediately
 * and returns that count
 * - before each blocking IPC wait we drain HW RX FIFO into ring buffer so
 * incoming bytes are not lost
 *
 * Note: this code assumes UART0 base is at 0x40034000 (RP2040). Adjust
 * uart_base_addr if needed.
 *
 * Keep in mind: this driver does not configure GPIO pin muxing. Make sure pins
 * are set to UART function before calling uart_init() (or add IO_BANK/GPIO
 * writes to do that here).
 */

#include <errno.h>
#include <l4/ipc.h>
#include <l4/schedule.h>
#include <l4/utcb.h>
#include <service.h>
#include <stddef.h>
#include <stdint.h>
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

static unsigned char *const uart_start = (unsigned char *)0x40034000;

static inline volatile uint32_t *uart_reg(unsigned off)
{
    return (volatile uint32_t *)(uart_start + off);
}

#define UART_FR_TXFF (1u << 5)
#define UART_FR_RXFE (1u << 4)

#define UART_CR_UARTEN (1u << 0)
#define UART_CR_TXE (1u << 8)
#define UART_CR_RXE (1u << 9)

#define UART_LCRH_FEN (1u << 4)
#define UART_LCRH_WLEN_8 (3u << 5)

/* Reference peripheral clock for UART (adjust if your clk_peri is different) */
#ifndef UART_REF_CLK_HZ
#define UART_REF_CLK_HZ (12000000u)
#endif

#define RX_RING_CAP IPC_BUFFER_SIZE

struct rx_ring
{
    unsigned char buf[RX_RING_CAP];
    unsigned head;
    unsigned tail;
};

static struct rx_ring rxrb = {.head = 0, .tail = 0};

static inline unsigned rxrb_len(const struct rx_ring *r)
{
    if (r->head >= r->tail)
        return r->head - r->tail;
    return RX_RING_CAP - (r->tail - r->head);
}

static inline unsigned rxrb_free(const struct rx_ring *r)
{
    return RX_RING_CAP - rxrb_len(r) -
           1; /* keep one slot empty to disambiguate full/empty */
}

static inline void rxrb_push(struct rx_ring *r, unsigned char c)
{
    unsigned next = (r->head + 1) % RX_RING_CAP;
    if (next == r->tail)
    {
        /* buffer full: drop oldest to make space */
        r->tail = (r->tail + 1) % RX_RING_CAP;
    }
    r->buf[r->head] = c;
    r->head = next;
}

static inline int rxrb_pop(struct rx_ring *r)
{
    if (r->tail == r->head)
        return -1;
    unsigned char c = r->buf[r->tail];
    r->tail = (r->tail + 1) % RX_RING_CAP;
    return c;
}

/* ----- IPC helper buffer & utcb ----- */
extern L4_utcb_t __utcb;
static unsigned char ipc_buffer[IPC_BUFFER_SIZE];

static void make_ipc_error(int err_no)
{
    L4_load_mr(
        0, (L4_msg_tag_t){.u = 1, .t = 0, .flags = 0, .label = TERM_ERROR}.raw);
    L4_load_mr(1, err_no);
}

/* ----- UART hardware helpers ----- */

static void uart_set_baud(uint32_t uart_clk_hz, uint32_t baud)
{
    volatile uint32_t *ibrd = uart_reg(PL011_UARTIBRD);
    volatile uint32_t *fbrd = uart_reg(PL011_UARTFBRD);
    volatile uint32_t *lcrh = uart_reg(PL011_UARTLCR_H);

    uint64_t scaled = (uint64_t)uart_clk_hz * 4u;
    uint32_t divider = (uint32_t)((scaled + baud / 2u) / baud);

    uint32_t int_part = divider >> 6;
    uint32_t frac_part = divider & 0x3Fu;

    *ibrd = int_part;
    *fbrd = frac_part;

    *lcrh = (3u << 5) | (1u << 4);
}

/* Minimal uart init (disable -> set baud -> 8N1 FIFO -> enable).
   Non-invasive: does not touch GPIO muxing. */
static void uart_init(uint32_t baud)
{
    /* Disable UART while configuring */
    *uart_reg(PL011_UARTCR) = 0;

    /* Program baud */
    uart_set_baud(UART_REF_CLK_HZ, baud);

    /* 8 bits, no parity, 1 stop, enable FIFOs */
    *uart_reg(PL011_UARTLCR_H) = (UART_LCRH_WLEN_8 | UART_LCRH_FEN);

    /* Enable TX and RX and the UART */
    *uart_reg(PL011_UARTCR) = (UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE);
}

/* Drain HW RX FIFO into rx ring buffer. Call frequently to avoid losing bytes.
 */
static void uart_drain_hw_rx_to_ring(void)
{
    volatile uint32_t *fr = uart_reg(PL011_UARTFR);
    volatile uint32_t *dr = uart_reg(PL011_UARTDR);

    while (!(*fr & UART_FR_RXFE))
    {
        unsigned char c = (unsigned)(*dr & 0xffu);
        if (rxrb_free(&rxrb) == 0)
        {
            /* ring full: drop the byte (or drop oldest by pushing) */
            /* We choose to push which will overwrite oldest via rxrb_push logic
             */
        }
        rxrb_push(&rxrb, c);
    }
}

/* Non-blocking write: write up to len bytes into HW TX FIFO immediately.
   Return number actually written. */
static size_t uart_write_nonblocking(const void *buffer, size_t len)
{
    const unsigned char *p = (const unsigned char *)buffer;
    volatile uint32_t *fr = uart_reg(PL011_UARTFR);
    volatile uint32_t *dr = uart_reg(PL011_UARTDR);
    size_t written = 0;

    for (size_t i = 0; i < len; ++i)
    {
        if (*fr & UART_FR_TXFF)
            break; /* FIFO full - stop and return how many bytes we wrote */
        *dr = (uint32_t)p[i];
        ++written;
    }
    return written;
}

/* Non-blocking read: copy up to len bytes from rx ring into 'buffer'.
   Returns number of bytes copied.
   If ring empty, returns 0. */
static size_t uart_read_from_ring(void *buffer, size_t len)
{
    unsigned char *out = (unsigned char *)buffer;
    size_t copied = 0;
    while (copied < len)
    {
        int c = rxrb_pop(&rxrb);
        if (c < 0)
            break;
        out[copied++] = (unsigned char)c;
    }
    return copied;
}

/* ----- GPIO configuration ----- */

#define IO_BANK0_BASE 0x40014000
#define PADS_BANK0_BASE 0x4001C000

#define GPIO_CTRL_OFFSET 0x04
#define GPIO_PADS_OFFSET 0x04

#define GPIO0_CTRL (IO_BANK0_BASE + 0 * 8 + GPIO_CTRL_OFFSET)
#define GPIO1_CTRL (IO_BANK0_BASE + 1 * 8 + GPIO_CTRL_OFFSET)

#define GPIO0_PADS (PADS_BANK0_BASE + 0 * 4 + GPIO_PADS_OFFSET)
#define GPIO1_PADS (PADS_BANK0_BASE + 1 * 4 + GPIO_PADS_OFFSET)

static inline void mmio_write(unsigned addr, unsigned val)
{
    *(volatile unsigned *)addr = val;
}

static void uart_gpio_init(void)
{
    /*
     * Disable pulls, enable input, normal drive strength.
     * This avoids fighting the USB-UART adapter.
     */
    mmio_write(GPIO0_PADS, 0); // TX
    mmio_write(GPIO1_PADS, 0); // RX

    /*
     * Set function select = UART0 (func 2)
     * CTRL register layout:
     * bits [4:0] = FUNCSEL
     */
    mmio_write(GPIO0_CTRL, 2); // GPIO0 → UART0_TX
    mmio_write(GPIO1_CTRL, 2); // GPIO1 → UART0_RX
}

/* ----- Hardware Block Reset ----- */

#define RESETS_BASE 0x4000C000
#define RESETS_RESET (*(volatile uint32_t *)(RESETS_BASE + 0x00))
#define RESETS_RESET_DONE (*(volatile uint32_t *)(RESETS_BASE + 0x08))

#define UART0_RESET_BIT (1u << 12)

static void uart0_hw_reset(void)
{
    // 1. Assert UART0 reset
    RESETS_RESET |= UART0_RESET_BIT;
    while ((RESETS_RESET_DONE & UART0_RESET_BIT) != 0)
        L4_yield();

    // 2. De-assert UART0 reset
    RESETS_RESET &= ~UART0_RESET_BIT;
    while ((RESETS_RESET_DONE & UART0_RESET_BIT) == 0)
        L4_yield();
}

/* ----- IPC-facing implementations (fill TODOs) ----- */

/* Return value semantics for pl011_handle_read_impl:
 *   returns number of bytes *not* copied (so caller computes ret_len = len -
 * ret) (Kept same as your original code where pl011_handle_read_impl returned
 * 'ret' and then ret_len = len - ret; but we will return number of bytes NOT
 * copied? To be consistent with your earlier logic: original had const unsigned
 * ret = pl011_handle_read_impl(&ipc_buffer, len); const unsigned ret_len = len
 * - ret; So original expected pl011_handle_read_impl to return bytes NOT
 * copied? That was ambiguous. To make it logical: we'll make
 * pl011_handle_read_impl return the number of bytes copied. Then in
 * pl011_handle_read() we'll compute ret_len = ret and adjust how MR's are set.
 *
 * To avoid changing calling convention too much, we'll adapt the call-site
 * below to expect 'copied'.
 */

/* We'll implement pl011_handle_read_impl to:
   1) drain HW FIFO to ring
   2) copy up to len bytes from ring into buffer
   3) return number of bytes copied
*/
static unsigned pl011_handle_read_impl(void *buffer, size_t len)
{
    if (len == 0)
        return 0;

    /* First, pull bytes from HW FIFO into ring */
    uart_drain_hw_rx_to_ring();

    /* Now copy from ring to user buffer */
    size_t copied = uart_read_from_ring(buffer, len);
    return (unsigned)copied;
}

/* Write implementation: attempt to write as many bytes as possible into HW
   FIFO, returning number written. This is non-blocking: it never spins waiting
   for FIFO space. */
static unsigned pl011_handle_write_impl(void *buffer, size_t len)
{
    if (len == 0)
        return 0;
    size_t written = uart_write_nonblocking(buffer, len);
    return (unsigned)written;
}

/* ----- high-level IPC handlers (adjusted to use the above semantics) ----- */

static void pl011_handle_read(L4_msg_tag_t tag, L4_thread_id from)
{
    (void)from;
    if (tag.u != 2 || tag.t != 0)
    {
        make_ipc_error(EINVAL);
        return;
    }
    unsigned req_len;
    L4_store_mr(TERM_READ_SIZE, &req_len);
    if (req_len == 0)
    {
        /* nothing requested */
        const struct L4_simple_string_item empty_item = {
            .c = 0,
            .type = L4_data_type_string_item,
            .compound = 0,
            .length = 0,
            .ptr = (unsigned)&ipc_buffer};
        L4_load_mr(
            TERM_READ_RET_OP,
            (L4_msg_tag_t){.u = 0, .t = 2, .flags = 0, .label = TERM_READ_RET}
                .raw);
        L4_load_mrs(TERM_READ_RET_STR, 2, (unsigned *)&empty_item);
        return;
    }

    /* Perform the non-blocking read (copies into ipc_buffer) */
    const unsigned copied = pl011_handle_read_impl(&ipc_buffer, req_len);

    const struct L4_simple_string_item item = {.c = 0,
                                               .type = L4_data_type_string_item,
                                               .compound = 0,
                                               .length = copied,
                                               .ptr = (unsigned)&ipc_buffer};
    L4_load_mr(
        TERM_READ_RET_OP,
        (L4_msg_tag_t){.u = 0, .t = 2, .flags = 0, .label = TERM_READ_RET}.raw);
    L4_load_mrs(TERM_READ_RET_STR, 2, (unsigned *)&item);
}

/* Write handler: client sent a string_item describing buffer+len. We copy from
   client buffer into HW FIFO (as much as will fit now) and return write count.
 */
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

    /* Safety: clamp length to IPC_BUFFER_SIZE to avoid copying huge memory */
    size_t to_write = string_item.length;
    if (to_write > IPC_BUFFER_SIZE)
        to_write = IPC_BUFFER_SIZE;

    /* If the client's data pointer is actually accessible directly, we can pass
       it to uart_write_nonblocking. Otherwise a secure driver would copy to a
       local buffer first. Here we assume the pointer is in client's space but
       L4_store_mrs gave us a copy of the descriptor only; we will copy into
       ipc_buffer first. */
    memcpy(ipc_buffer, (const void *)string_item.ptr, to_write);

    const unsigned wrote =
        pl011_handle_write_impl((void *)ipc_buffer, to_write);

    L4_load_mr(
        TERM_WRITE_RET_OP,
        (L4_msg_tag_t){.u = 1, .t = 0, .flags = 0, .label = TERM_WRITE_RET}
            .raw);
    L4_load_mr(TERM_WRITE_RET_SIZE, wrote);
}

/* ----- service registration (unchanged) ----- */
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

/* ----- main loop: process HW drain then IPC ----- */
int main()
{
    uart0_hw_reset();
    uart_gpio_init();
    /* Initialize UART with a common baud (change if you configured clocks
     * differently) */
    uart_init(115200);

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
        /* Before blocking for IPC, drain any bytes from HW FIFO into rx ring
         * buffer */
        uart_drain_hw_rx_to_ring();

        /* Prepare to accept string items from clients */
        L4_load_br(0, acceptor.raw);
        L4_load_brs(1, 2, (unsigned *)&string_acceptor);

        /* Block waiting for a message from any thread */
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

        /* Send reply to the client (previously prepared by handlers) */
        tag =
            L4_ipc(from, L4_NILTHREAD, L4_timeouts(L4_never, L4_never), &from);
        if (L4_ipc_failed(tag))
        {
            continue;
        }
    }
}

/* minimal _start wrapper to call main like in your original code */
__attribute__((naked)) void _start()
{
    asm("pop {r0, r1}\n\t"
        "movs r9, r1\n\t"
        "movs r2, #0\n\t"
        "movs lr, r2\n\t"
        "bl %c[main]\n\t" ::[main] ""(main));
}
