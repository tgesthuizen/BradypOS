#include "builtins.h"
#include "parser.h"
#include <l4/ipc.h>
#include <l4/schedule.h>
#include <l4/thread.h>
#include <l4/utcb.h>
#include <service.h>
#include <string.h>
#include <term.h>
#include <vfs.h>
#include <vfs/client.h>

enum
{
    IPC_BUFFER_SIZE = 128,
};

register unsigned char *got_location asm("r9");

extern L4_utcb_t __utcb;
unsigned char ipc_buffer[IPC_BUFFER_SIZE];
static char filename_buf[VFS_PATH_MAX];
L4_thread_id term_service;
L4_thread_id romfs_service;

static L4_thread_id fetch_service(const char *svc)
{
    L4_thread_id root_thread = __utcb.t_pager;
    union
    {
        char c[4];
        unsigned u;
    } v;
    memcpy(&v.c, svc, sizeof(v.c));
    L4_load_mr(
        SERV_GET_OP,
        (L4_msg_tag_t){.u = 1, .t = 0, .flags = 0, .label = SERV_GET}.raw);
    L4_load_mr(SERV_GET_NAME, v.u);
    L4_thread_id from;
    L4_msg_tag_t tag = L4_ipc(root_thread, root_thread,
                              L4_timeouts(L4_never, L4_never), &from);
    if (L4_ipc_failed(tag) || tag.label != SERV_GET_RET || tag.u != 1 ||
        tag.t != 0)
    {
        return L4_NILTHREAD;
    }
    unsigned tid;
    L4_store_mr(SERV_GET_RET_TID, &tid);
    return (L4_thread_id)tid;
}

static L4_thread_id wait_for_service(const char *svc)
{
    L4_thread_id tid = L4_NILTHREAD;
    tid = fetch_service(svc);
    while (L4_is_nil_thread(tid))
    {
        L4_yield();
        tid = fetch_service(svc);
    }
    return tid;
}

enum
{
    ROOT_FD = 3,
    ETC_FD = 4,
    MOTD_FD = 5,
    VERSION_FD = 6,
};

bool term_write(L4_thread_id term_service, const unsigned char *buf,
                size_t size)
{
    while (size)
    {
        L4_load_mr(
            TERM_WRITE_OP,
            (L4_msg_tag_t){.u = 1, .t = 2, .flags = 0, .label = TERM_WRITE}
                .raw);
        L4_load_mr(TERM_WRITE_FLAGS, 0);
        struct L4_simple_string_item item = {.c = 0,
                                             .type = L4_data_type_string_item,
                                             .compound = 0,
                                             .length = size,
                                             .ptr = (unsigned)buf};
        L4_load_mrs(TERM_WRITE_BUF, 2, (unsigned *)&item);
        L4_thread_id from;
        L4_msg_tag_t answer_tag = L4_ipc(
            term_service, term_service, L4_timeouts(L4_never, L4_never), &from);
        if (L4_ipc_failed(answer_tag) && answer_tag.label != TERM_WRITE_RET)
        {
            return false;
        }
        unsigned written;
        L4_store_mr(TERM_WRITE_RET_SIZE, &written);
        buf += written;
        size -= written;
    }
    return true;
}

static void write_buffer(const unsigned char *buf, size_t size)
{
    // Chop the file up into 64 byte chunks
    unsigned offset = 0;
    while (offset < size)
    {
        unsigned payload_size = size - offset;
        if (payload_size > 64)
            payload_size = 64;
        term_write(term_service, buf + offset, payload_size);
        offset += payload_size;
    }
}

static void startup()
{
    static const unsigned char init_space_out[] = "\n";
    write_buffer(init_space_out, sizeof(init_space_out));
    static const char etc_path[] = "etc";
    if (!open_at(romfs_service, ROOT_FD, ETC_FD, etc_path,
                 sizeof(etc_path) - 1))
    {
        return;
    }
    {
        static const char motd_path[] = "motd.txt";
        if (!open_at(romfs_service, ETC_FD, MOTD_FD, motd_path,
                     sizeof(motd_path) - 1))
        {
            return;
        }
        struct vfs_stat_result motd_stat =
            stat(romfs_service, MOTD_FD, filename_buf);
        if (!motd_stat.success)
        {
            return;
        }
        struct map_result res = map(romfs_service, MOTD_FD, 0, motd_stat.size);
        if (res.addr == NULL)
        {
            return;
        }
        write_buffer(res.addr, motd_stat.size);
        close(romfs_service, MOTD_FD);
    }
    {
        static const char version_path[] = "version";
        if (!open_at(romfs_service, ETC_FD, VERSION_FD, version_path,
                     sizeof(version_path) - 1))
        {
            return;
        }
        struct vfs_stat_result version_stat =
            stat(romfs_service, VERSION_FD, filename_buf);
        if (!version_stat.success)
        {
            return;
        }
        struct map_result res =
            map(romfs_service, VERSION_FD, 0, version_stat.size);
        if (res.addr == NULL)
        {
            return;
        }
        write_buffer(res.addr, version_stat.size);
    }

    close(romfs_service, ETC_FD);
}

static unsigned char line_buffer[MAX_COMMAND_LINE_LEN + 1];
static size_t line_pos = 0;
static size_t line_len = 0;

static bool readline(L4_thread_id term_service)
{
    struct L4_simple_string_item item = {.c = 0,
                                         .type = L4_data_type_string_item,
                                         .compound = 0,
                                         .length = IPC_BUFFER_SIZE,
                                         .ptr = (unsigned)&ipc_buffer};

    L4_load_br(0, L4_string_items_acceptor.raw);
    L4_load_brs(1, 2, (unsigned *)&item);
    L4_load_mr(
        TERM_READ_OP,
        (L4_msg_tag_t){.u = 2, .t = 0, .flags = 0, .label = TERM_READ}.raw);
    L4_load_mr(TERM_READ_FLAGS, 0);
    L4_load_mr(TERM_READ_SIZE, IPC_BUFFER_SIZE);

    L4_thread_id from;
    L4_msg_tag_t answer_tag = L4_ipc(term_service, term_service,
                                     L4_timeouts(L4_never, L4_never), &from);
    if (L4_ipc_failed(answer_tag) || answer_tag.label != TERM_READ_RET)
        return false;

    L4_store_mrs(TERM_READ_RET_STR, 2, (unsigned *)&item);

    unsigned char *buf = (unsigned char *)item.ptr;
    for (size_t i = 0; i < item.length; i++)
    {
        unsigned char c = buf[i];

        if (c == '\r' || c == '\n')
        {
            if (line_pos < MAX_COMMAND_LINE_LEN - 2)
            {
                line_buffer[line_pos++] = '\n';
                line_buffer[line_pos] = '\0';
                line_len = line_pos;
                line_pos = 0;
                return true;
            }
        }
        else if (c == 0x7f || c == '\b')
        {
            if (line_pos > 0)
                line_pos--;
        }
        else if (line_pos < MAX_COMMAND_LINE_LEN - 3)
        {
            line_buffer[line_pos++] = c;
        }
    }

    return false;
}

int getch()
{
    if (line_pos >= line_len)
    {
        return EOF;
    }
    return line_buffer[line_pos++];
}

void ungetch(int ch)
{
    // We accept @p c for compatibility with the standard C library. With our
    // implementation, we do not rely on its value.
    (void)ch;
    --line_pos;
}

void exec_command(char **args)
{
    for (const struct bsh_builtin **builtin = builtins; *builtin; ++builtin)
    {
        if (strcmp(args[0], (*builtin)->cmd) == 0)
        {
            (*builtin)->handler(args);
            return;
        }
    }
    static const unsigned char invalid_msg[] = "Invalid command";
    term_write(term_service, invalid_msg, sizeof invalid_msg);
}

int main()
{
    term_service = wait_for_service("term");
    romfs_service = wait_for_service("rom\0");

    if (!open_root_fd(romfs_service, ROOT_FD))
    {
        // TODO: Do something
    }
    startup();

    while (1)
    {
        term_write(term_service, (const unsigned char *)"> ", 2);

        line_pos = 0;
        while (!readline(term_service))
        {
            L4_yield();
        }

        parse();
    }

    while (1)
    {
        L4_yield();
    }
}

__attribute__((naked)) void _start()
{
    asm volatile("pop {r0, r1}\n\t"
                 "movs r9, r1\n\t"
                 "movs r2, #0\n\t"
                 "movs lr, r2\n\t"
                 "bl %c[main]\n\t" ::[main] ""(main));
}
