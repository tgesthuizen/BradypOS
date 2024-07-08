#include <l4/ipc.h>
#include <l4/schedule.h>
#include <l4/thread.h>
#include <l4/utcb.h>
#include <service.h>
#include <string.h>
#include <term.h>
#include <vfs.h>

enum
{
    IPC_BUFFER_SIZE = 128,
};

register unsigned char *got_location asm("r9");

extern L4_utcb_t __utcb;
static unsigned char ipc_buffer[IPC_BUFFER_SIZE];
static L4_thread_id term_service;
static L4_thread_id romfs_service;

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

static bool open_root_fd(L4_thread_id romfs_server, int fd)
{
    L4_msg_tag_t msg_tag = {.flags = 0, .label = VFS_OPENROOT, .t = 0, .u = 1};
    L4_thread_id from;
    L4_load_mr(VFS_OPENROOT_OP, msg_tag.raw);
    L4_load_mr(VFS_OPENROOT_FD, (unsigned)fd);
    L4_msg_tag_t answer_tag = L4_ipc(romfs_server, romfs_server,
                                     L4_timeouts(L4_never, L4_never), &from);
    if (L4_ipc_failed(answer_tag))
    {
        return false;
    }
    if (answer_tag.label != VFS_OPENROOT_RET)
    {
        return false;
    }
    if (answer_tag.u != 1 || answer_tag.t != 0)
    {
        return false;
    }

    return true;
}

static bool open_at(L4_thread_id romfs_server, int fd, int new_fd,
                    const char *path, size_t pathlen)
{
    const L4_msg_tag_t msg_tag = {
        .flags = 0, .label = VFS_OPENAT, .u = 2, .t = 2};
    const struct L4_simple_string_item ssi = {.c = 0,
                                              .compound = 0,
                                              .type = L4_data_type_string_item,
                                              .length = pathlen,
                                              .ptr = (unsigned)path};
    L4_thread_id from;
    L4_load_mr(VFS_OPENAT_OP, msg_tag.raw);
    L4_load_mr(VFS_OPENAT_FD, fd);
    L4_load_mr(VFS_OPENAT_NEWFD, new_fd);
    L4_load_mrs(VFS_OPENAT_STR, 2, (unsigned *)&ssi);
    const L4_msg_tag_t answer_tag = L4_ipc(
        romfs_server, romfs_server, L4_timeouts(L4_never, L4_never), &from);
    if (L4_ipc_failed(answer_tag) || answer_tag.label != VFS_OPENAT_RET ||
        answer_tag.u != 1 || answer_tag.t != 0)
    {
        return false;
    }
    return true;
}

static bool close(L4_thread_id romfs_server, int fd)
{
    L4_load_mr(
        VFS_CLOSE_OP,
        (L4_msg_tag_t){.u = 1, .t = 0, .flags = 0, .label = VFS_CLOSE}.raw);
    L4_load_mr(VFS_CLOSE_FD, fd);
    L4_thread_id from;
    const L4_msg_tag_t answer_tag = L4_ipc(
        romfs_server, romfs_server, L4_timeouts(L4_never, L4_never), &from);
    return !L4_ipc_failed(answer_tag) && answer_tag.label == VFS_CLOSE_RET;
}

struct vfs_stat_result
{
    bool success;
    enum vfs_file_type type;
    size_t size;
};

static struct vfs_stat_result stat(L4_thread_id romfs_server, int fd)
{
    L4_load_mr(
        VFS_STAT_OP,
        (L4_msg_tag_t){.u = 1, .t = 0, .flags = 0, .label = VFS_STAT}.raw);
    L4_load_mr(VFS_STAT_FD, fd);
    L4_thread_id from;
    struct vfs_stat_result result = {
        .success = false, .type = VFS_FT_OTHER, .size = 0};
    const L4_msg_tag_t answer_tag = L4_ipc(
        romfs_server, romfs_server, L4_timeouts(L4_never, L4_never), &from);
    if (L4_ipc_failed(answer_tag) || answer_tag.label != VFS_STAT_RET)
    {
        return result;
    }

    unsigned result_type;
    L4_store_mr(VFS_STAT_RET_TYPE, &result_type);
    result.type = result_type;
    L4_store_mr(VFS_STAT_RET_SIZE, &result.size);
    result.success = true;
    return result;
}

struct map_result
{
    L4_fpage_t fpage;
    void *addr;
};

static struct map_result map(L4_thread_id romfs_server, int fd, size_t offset,
                             size_t size)
{
    const struct map_result invalid_result = {.fpage = L4_nilpage,
                                              .addr = NULL};
    L4_load_mr(
        VFS_MAP_OP,
        (L4_msg_tag_t){.u = 4, .t = 0, .flags = 0, .label = VFS_MAP}.raw);
    L4_load_mr(VFS_MAP_FD, fd);
    L4_load_mr(VFS_MAP_OFFSET, offset);
    L4_load_mr(VFS_MAP_SIZE, size);
    L4_load_mr(VFS_MAP_PERM, L4_readable);
    L4_thread_id from;
    const L4_msg_tag_t answer_tag = L4_ipc(
        romfs_server, romfs_server, L4_timeouts(L4_never, L4_never), &from);
    if (L4_ipc_failed(answer_tag) || answer_tag.label != VFS_MAP_RET ||
        answer_tag.u != 1 || answer_tag.t != 2)
        return invalid_result;
    struct map_result result = invalid_result;
    struct L4_map_item map_item;
    unsigned address;
    L4_store_mr(VFS_MAP_RET_ADDR, &address);
    L4_store_mrs(VFS_MAP_RET_MAP_ITEM, 2, (unsigned *)&map_item);
    result.addr = (void *)address;
    result.fpage = L4_map_item_snd_fpage(&map_item);
    return result;
}

enum
{
    ROOT_FD = 3,
    ETC_FD = 4,
    MOTD_FD = 5,
    VERSION_FD = 6,
};

static bool term_write(L4_thread_id term_service, const unsigned char *buf,
                       size_t size)
{
    L4_load_mr(
        TERM_WRITE_OP,
        (L4_msg_tag_t){.u = 1, .t = 2, .flags = 0, .label = TERM_WRITE}.raw);
    L4_load_mr(TERM_WRITE_FLAGS, 0);
    struct L4_simple_string_item item = {.c = 0,
                                         .type = L4_data_type_string_item,
                                         .compound = 0,
                                         .length = size,
                                         .ptr = (unsigned)buf};
    L4_load_mrs(TERM_WRITE_BUF, 2, (unsigned *)&item);
    L4_thread_id from;
    L4_msg_tag_t answer_tag = L4_ipc(term_service, term_service,
                                     L4_timeouts(L4_never, L4_never), &from);
    return !L4_ipc_failed(answer_tag) && answer_tag.label == TERM_WRITE_RET;
}

static void write_buffer(unsigned char *buf, size_t size)
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
        struct vfs_stat_result motd_stat = stat(romfs_service, MOTD_FD);
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
        struct vfs_stat_result version_stat = stat(romfs_service, VERSION_FD);
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
        L4_load_br(0, L4_string_items_acceptor.raw);
        struct L4_simple_string_item item = {.c = 0,
                                             .type = L4_data_type_string_item,
                                             .compound = 0,
                                             .length = IPC_BUFFER_SIZE,
                                             .ptr = (unsigned)&ipc_buffer};
        L4_load_brs(1, 2, (unsigned *)&item);
        L4_load_mr(
            TERM_READ_OP,
            (L4_msg_tag_t){.u = 2, .t = 0, .flags = 0, .label = TERM_READ}.raw);
        L4_load_mr(TERM_READ_FLAGS, 0);
        L4_load_mr(TERM_READ_SIZE, IPC_BUFFER_SIZE);
        L4_thread_id from;
        L4_msg_tag_t answer_tag = L4_ipc(
            term_service, term_service, L4_timeouts(L4_never, L4_never), &from);
        if (L4_ipc_failed(answer_tag) || answer_tag.label != TERM_READ_RET)
            break;
        L4_store_mrs(TERM_READ_RET_STR, 2, (unsigned *)&item);
        term_write(term_service, (const unsigned char *)item.ptr, item.length);
    }

    while (1)
    {
        L4_yield();
    }
}

__attribute__((naked)) void _start()
{
    asm volatile ("pop {r0, r1}\n\t"
        "movs r9, r1\n\t"
        "movs r2, #0\n\t"
        "movs lr, r2\n\t"
		  "bl %c[main]\n\t" :: [main]""(main));
}
