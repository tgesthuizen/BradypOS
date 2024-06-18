#include "variables.h"
#include <errno.h>
#include <l4/bootinfo.h>
#include <l4/ipc.h>
#include <l4/kip.h>
#include <l4/schedule.h>
#include <l4/space.h>
#include <l4/thread.h>
#include <l4/time.h>
#include <l4/utcb.h>
#include <romfs.h>
#include <string.h>
#include <vfs.h>

static __attribute__((noreturn)) void delete_romfs_thread()
{
    L4_thread_control(romfs_thread_id, L4_NILTHREAD, -1, -1, (void *)-1);
    while (1)
        ;
}

static __attribute__((noreturn)) void romfs_error(char *fmt, ...)
{
    (void)fmt;
    // TODO: Print string somehow
    delete_romfs_thread();
}

static unsigned char *rootfs_base;

static void locate_romfs_start(L4_kip_t *the_kip)
{
    struct L4_boot_info_header *const hdr =
        L4_read_kip_ptr(the_kip, the_kip->boot_info);
    if (hdr->magic != L4_BOOT_INFO_MAGIC)
        romfs_error("Could not locate boot info\n");
    struct L4_boot_info_variables *entry =
        (struct L4_boot_info_variables *)((unsigned char *)hdr +
                                          hdr->first_entry);
    while (entry->offset_next != 0 && entry->type != L4_BOOT_INFO_VARIABLES)
    {
        entry = (struct L4_boot_info_variables *)((unsigned char *)entry +
                                                  entry->offset_next);
    }
    if (entry->type != L4_BOOT_INFO_VARIABLES)
        romfs_error("Boot info does not contain a variables record");
    unsigned var_idx = 0;
    struct L4_boot_info_variables_entry *var = entry->entries;
    static const char romfs_entry_name[] = "rootfs_base";
    while (var_idx < entry->variable_count &&
           memcmp(romfs_entry_name, var->name, sizeof(romfs_entry_name) - 1) !=
               0)
    {
        ++var;
        ++var_idx;
    }
    if (var_idx == entry->variable_count)
    {
        romfs_error("Boot variables do not contain a romfs entry record\n");
    }
    rootfs_base = (unsigned char *)var->value;
}

static int romfs_map(void **data, size_t offset, size_t size, void *user)
{
    (void)user;
    (void)size;
    *data = (unsigned char *)rootfs_base + offset;
    return 0;
}

static void romfs_unmap(void **data, size_t size, void *user)
{
    (void)data;
    (void)size;
    (void)user;
}

static const struct romfs_block_iface romfs_ops = {.map = romfs_map,
                                                   .unmap = romfs_unmap};

enum hash_item_status
{
    HASH_ITEM_FREE,
    HASH_ITEM_USED,
    HASH_ITEM_TOMBSTONE,
};

struct vfs_file_state
{
    enum hash_item_status status;
    L4_thread_id owner;
    int fd;
    struct romfs_file_info file_info;
    size_t file_off;
};

enum
{
    FILE_STATE_LOG = 5,
    FILE_STATE_COUNT = 1 << FILE_STATE_LOG,
};

struct vfs_file_state file_states[FILE_STATE_COUNT];

__attribute__((noreturn, naked)) void romfs_start()
{
    asm("pop {r0, r1}\n\t"
        "movs r9, r1\n\t"
        "b romfs_main\n\t");
}

static char filename_buf[VFS_PATH_MAX + 1];

static unsigned hash_combine(unsigned a, unsigned b)
{
    a ^= b + 0x9e3779b9 + (a << 6) + (a >> 2);
    return a;
}

static struct vfs_file_state *insert_vfs_file_state(L4_thread_id owner, int fd)
{
    const unsigned initial_idx = hash_combine(owner, fd) % FILE_STATE_COUNT;
    unsigned idx = initial_idx;
    for (unsigned i = 0; i < FILE_STATE_LOG;
         ++i, idx = (initial_idx + i * i) % FILE_STATE_COUNT)
    {
        if (file_states[idx].status == HASH_ITEM_FREE ||
            file_states[idx].status == HASH_ITEM_TOMBSTONE)
        {
            file_states[idx] = (struct vfs_file_state){.status = HASH_ITEM_USED,
                                                       .owner = owner,
                                                       .fd = fd,
                                                       .file_off = 0};
            return file_states + idx;
        }
    }
    return NULL;
}

static bool delete_vfs_file_state(L4_thread_id owner, int fd)
{
    const unsigned initial_idx = hash_combine(owner, fd) % FILE_STATE_COUNT;
    unsigned idx = initial_idx;
    for (unsigned i = 0; i < FILE_STATE_LOG;
         ++i, idx = (initial_idx + i * i) % FILE_STATE_COUNT)
    {
        if (file_states[idx].status == HASH_ITEM_FREE)
        {
            break;
        }
        if (file_states[idx].status == HASH_ITEM_TOMBSTONE ||
            file_states[idx].owner != owner || file_states[idx].fd != fd)
        {
            continue;
        }
        file_states[idx].status = HASH_ITEM_TOMBSTONE;
        return true;
    }
    return false;
}

static struct vfs_file_state *find_vfs_file_state(L4_thread_id owner, int fd)
{
    const unsigned initial_idx = hash_combine(owner, fd) % FILE_STATE_COUNT;
    unsigned idx = initial_idx;
    for (unsigned i = 0; i < FILE_STATE_LOG;
         ++i, idx = (initial_idx + i * i) % FILE_STATE_COUNT)
    {
        if (file_states[idx].status == HASH_ITEM_FREE)
        {
            break;
        }
        if (file_states[idx].status == HASH_ITEM_TOMBSTONE ||
            file_states[idx].owner != owner || file_states[idx].fd != fd)
        {
            continue;
        }
        return file_states + idx;
    }
    return NULL;
}

static void make_ipc_error(int errno)
{
    L4_msg_tag_t msg_tag;
    msg_tag.flags = 0;
    msg_tag.label = VFS_ERROR;
    msg_tag.u = 1;
    msg_tag.t = 0;
    romfs_server_utcb->mr[0] = msg_tag.raw;
    romfs_server_utcb->mr[1] = errno;
}

static void handle_vfs_openroot(L4_thread_id from, L4_msg_tag_t msg_tag)
{
    if (msg_tag.u != 1 || msg_tag.t != 0)
    {
        make_ipc_error(EINVAL);
        return;
    }
    const int fd = (int)romfs_server_utcb->mr[VFS_OPENROOT_FD];
    struct vfs_file_state *const file_state = insert_vfs_file_state(from, fd);
    if (!file_state)
    {
        make_ipc_error(ENOMEM);
        return;
    }
    file_state->file_off = romfs_root_directory(&romfs_ops, NULL);
    if (file_state->file_off == ROMFS_INVALID_FILE)
    {
        make_ipc_error(EIO);
        return;
    }

    L4_msg_tag_t answer_tag;
    answer_tag.flags = 0;
    answer_tag.label = VFS_OPENROOT_RET;
    answer_tag.t = 0;
    answer_tag.u = 1;
    romfs_server_utcb->mr[0] = answer_tag.raw;
    romfs_server_utcb->mr[1] = (unsigned)fd;
}

static void handle_vfs_openat(L4_thread_id from, L4_msg_tag_t msg_tag)
{
    (void)from;
    if (msg_tag.u != 2 || msg_tag.t != 2)
    {
        make_ipc_error(EINVAL);
        return;
    }
    const int fd = (int)romfs_server_utcb->mr[VFS_OPENAT_FD];
    const int new_fd = (int)romfs_server_utcb->mr[VFS_OPENAT_NEWFD];
    struct L4_simple_string_item item;
    memcpy(&item, &romfs_server_utcb->mr[VFS_OPENAT_STR], sizeof(unsigned) * 2);
    if (item.type != L4_data_type_string_item)
    {
        make_ipc_error(EINVAL);
        return;
    }
    if (item.compound)
    {
        // TODO: This is really an internal problem and should not affect the
        // caller.
        make_ipc_error(EINVAL);
        return;
    }
    struct vfs_file_state *const dir = find_vfs_file_state(from, fd);
    if (dir == NULL)
    {
        make_ipc_error(EINVAL);
        return;
    }
    const size_t file_offset =
        romfs_openat(&romfs_ops, dir->file_off, (const char *)item.ptr, NULL);
    if (file_offset == ROMFS_INVALID_FILE)
    {
        make_ipc_error(ENOENT);
        return;
    }
    struct vfs_file_state *const new_state =
        insert_vfs_file_state(from, new_fd);
    if (new_state == NULL)
    {
        make_ipc_error(ENOMEM);
        return;
    }
    new_state->file_off = file_offset;

    romfs_server_utcb->mr[0] =
        (L4_msg_tag_t){.flags = 0, .label = VFS_OPENAT_RET, .u = 1, .t = 0}.raw;
    romfs_server_utcb->mr[1] = new_fd;
}

static void handle_vfs_close(L4_thread_id from, L4_msg_tag_t msg_tag)
{
    if (msg_tag.u != 1 || msg_tag.t != 0)
    {
        make_ipc_error(EINVAL);
        return;
    }
    const int fd = (int)romfs_server_utcb->mr[VFS_CLOSE_FD];
    if (delete_vfs_file_state(from, fd) == false)
    {
        make_ipc_error(ENOENT);
        return;
    }
    romfs_server_utcb->mr[0] =
        (L4_msg_tag_t){.flags = 0, .label = VFS_CLOSE_RET, .u = 0, .t = 0}.raw;
}

static void handle_vfs_read(L4_thread_id from, L4_msg_tag_t msg_tag)
{
    if (msg_tag.u != 3 || msg_tag.t != 0)
    {
        make_ipc_error(EINVAL);
        return;
    }
    const int fd = (int)romfs_server_utcb->mr[VFS_READ_FD];
    const unsigned offset = romfs_server_utcb->mr[VFS_READ_OFFSET];
    unsigned size = romfs_server_utcb->mr[VFS_READ_SIZE];
    struct vfs_file_state *const file_state = find_vfs_file_state(from, fd);
    if (file_state == NULL)
    {
        make_ipc_error(EINVAL);
        return;
    }

    const size_t content_offset =
        romfs_file_content_offset(&romfs_ops, file_state->file_off, NULL);
    const size_t remaining_file_size = file_state->file_info.size - offset;
    if (size > remaining_file_size)
        size = remaining_file_size;

    romfs_server_utcb->mr[0] =
        (L4_msg_tag_t){.label = VFS_READ_RET, .flags = 0, .u = 0, .t = 2}.raw;
    const struct L4_simple_string_item content_string = {
        .c = 0,
        .type = L4_data_type_string_item,
        .compound = 0,
        .length = size,
        .ptr = (unsigned)(rootfs_base + content_offset + offset)};
    memcpy(&romfs_server_utcb->mr[1], &content_string, sizeof(unsigned) * 2);
}

static void handle_vfs_write(L4_thread_id from, L4_msg_tag_t msg_tag)
{
    if (msg_tag.u != 3 || msg_tag.t != 2)
    {
        make_ipc_error(EINVAL);
        return;
    }
    const int fd = (int)romfs_server_utcb->mr[VFS_WRITE_FD];

    struct vfs_file_state *const file_state = find_vfs_file_state(from, fd);
    if (file_state == NULL)
    {
        make_ipc_error(ENOENT);
        return;
    }

    // We cannot write to ROMFS, so the result is always the same
    make_ipc_error(EROFS);
}

static L4_msg_buffer_t ipc_buffers;

__attribute__((noreturn)) void romfs_main(L4_utcb_t *utcb)
{
    (void)utcb;
    locate_romfs_start(the_kip);
    const L4_acceptor_t ipc_acceptor = L4_string_items_acceptor;
    ipc_buffers.raw[0] = ipc_acceptor.raw;
    const struct L4_simple_string_item filename_buf_item = {
        .type = L4_data_type_string_item,
        .length = VFS_PATH_MAX + 1,
        .c = 0,
        .compound = 0,
        .ptr = (unsigned)&filename_buf};
    memcpy(ipc_buffers.raw + 1, &filename_buf_item, sizeof(unsigned) * 2);
    while (1)
    {
        L4_thread_id from;
        L4_msg_tag_t msg_tag;
        memcpy(romfs_server_utcb->br, &ipc_buffers.raw, sizeof(unsigned) * 3);

        L4_ipc(L4_NILTHREAD, L4_ANYTHREAD, 0, &from);
        msg_tag.raw = romfs_server_utcb->mr[0];
        if (L4_ipc_failed(msg_tag))
        {
            // TODO: Log error about IPC error
            continue;
        }
        switch (msg_tag.label)
        {
        case VFS_OPENROOT:
            handle_vfs_openroot(from, msg_tag);
            break;
        case VFS_OPENAT:
            handle_vfs_openat(from, msg_tag);
            break;
        case VFS_CLOSE:
            handle_vfs_close(from, msg_tag);
            break;
        case VFS_READ:
            handle_vfs_read(from, msg_tag);
            break;
        case VFS_WRITE:
            handle_vfs_write(from, msg_tag);
            break;
        }

        L4_ipc(from, L4_NILTHREAD, L4_timeouts(L4_zero_time, L4_zero_time),
               &from);
        const L4_msg_tag_t answer_tag = {.raw = romfs_server_utcb->mr[0]};
        if (L4_ipc_failed(answer_tag))
        {
            // TODO: Log error about IPC error
            continue;
        }
    }
    // In case we reach the end of this program, delete us
    delete_romfs_thread();
}
