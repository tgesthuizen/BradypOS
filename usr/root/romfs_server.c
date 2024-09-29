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

static char filename_buf[VFS_PATH_MAX + 1];

static unsigned hash_combine(unsigned a, unsigned b)
{
    a ^= b + 0x9e3779b9 + (a << 6) + (a >> 2);
    return a;
}

static struct vfs_file_state *
find_vfs_file_state_insertion_point(L4_thread_id owner, int fd)
{
    const unsigned initial_idx = hash_combine(owner, fd) % FILE_STATE_COUNT;
    unsigned idx = initial_idx;
    for (unsigned i = 0; i < FILE_STATE_LOG;
         ++i, idx = (initial_idx + i * i) % FILE_STATE_COUNT)
    {
        if (file_states[idx].status == HASH_ITEM_FREE ||
            file_states[idx].status == HASH_ITEM_TOMBSTONE)
        {
            return file_states + idx;
        }
        if (file_states[idx].status == HASH_ITEM_USED &&
            file_states[idx].owner == owner && file_states[idx].fd == fd)
        {
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
    L4_load_mr(
        VFS_ERROR_RET_OP,
        (L4_msg_tag_t){.flags = 0, .label = VFS_ERROR, .u = 1, .t = 0}.raw);
    L4_load_mr(VFS_ERROR_RET_ERRNO, (unsigned)errno);
}

static void handle_vfs_openroot(L4_thread_id from, L4_msg_tag_t msg_tag)
{
    if (msg_tag.u != 1 || msg_tag.t != 0)
    {
        make_ipc_error(EINVAL);
        return;
    }
    int fd;
    L4_store_mr(VFS_OPENROOT_FD, (unsigned *)&fd);
    struct vfs_file_state *const file_state =
        find_vfs_file_state_insertion_point(from, fd);
    if (!file_state)
    {
        make_ipc_error(ENOMEM);
        return;
    }
    if (file_state->status == HASH_ITEM_USED)
    {
        make_ipc_error(EBADF);
        return;
    }
    const unsigned file_off = romfs_root_directory(&romfs_ops, NULL);
    if (file_off == ROMFS_INVALID_FILE)
    {
        make_ipc_error(EIO);
        return;
    }
    struct romfs_file_info info;
    if (!romfs_file_info(&romfs_ops, file_off, &info, NULL))
    {
        make_ipc_error(EIO);
        return;
    }
    *file_state = (struct vfs_file_state){.status = HASH_ITEM_USED,
                                          .owner = from,
                                          .fd = fd,
                                          .file_off = file_off,
                                          .file_info = info};
    L4_load_mr(
        VFS_OPENROOT_RET_OP,
        (L4_msg_tag_t){.flags = 0, .label = VFS_OPENROOT_RET, .t = 0, .u = 1}
            .raw);
    L4_load_mr(VFS_OPENROOT_RET_FD, (unsigned)fd);
}

static void handle_vfs_openat(L4_thread_id from, L4_msg_tag_t msg_tag)
{
    (void)from;
    if (msg_tag.u != 2 || msg_tag.t != 2)
    {
        make_ipc_error(EINVAL);
        return;
    }
    int fd;
    int new_fd;
    struct L4_simple_string_item item;
    L4_store_mr(VFS_OPENAT_FD, (unsigned *)&fd);
    L4_store_mr(VFS_OPENAT_NEWFD, (unsigned *)&new_fd);
    L4_store_mrs(VFS_OPENAT_STR, 2, (unsigned *)&item);
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
    ((char *)item.ptr)[item.length] = '\0';

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
        find_vfs_file_state_insertion_point(from, new_fd);
    if (new_state == NULL)
    {
        make_ipc_error(ENOMEM);
        return;
    }
    if (new_state->status == HASH_ITEM_USED)
    {
        make_ipc_error(EBADF);
        return;
    }
    struct romfs_file_info info;
    if (!romfs_file_info(&romfs_ops, file_offset, &info, NULL))
    {
        make_ipc_error(EIO);
        return;
    }
    *new_state = (struct vfs_file_state){.status = HASH_ITEM_USED,
                                         .owner = from,
                                         .fd = new_fd,
                                         .file_off = file_offset,
                                         .file_info = info};
    L4_load_mr(
        VFS_OPENAT_RET_OP,
        (L4_msg_tag_t){.flags = 0, .label = VFS_OPENAT_RET, .u = 1, .t = 0}
            .raw);
    L4_load_mr(VFS_OPENAT_RET_FD, (unsigned)new_fd);
}

static void handle_vfs_close(L4_thread_id from, L4_msg_tag_t msg_tag)
{
    if (msg_tag.u != 1 || msg_tag.t != 0)
    {
        make_ipc_error(EINVAL);
        return;
    }
    int fd;
    L4_store_mr(VFS_CLOSE_FD, (unsigned *)&fd);
    if (delete_vfs_file_state(from, fd) == false)
    {
        make_ipc_error(ENOENT);
        return;
    }
    L4_load_mr(
        VFS_CLOSE_RET_OP,
        (L4_msg_tag_t){.flags = 0, .label = VFS_CLOSE_RET, .u = 0, .t = 0}.raw);
}

static void handle_vfs_stat(L4_thread_id from, L4_msg_tag_t msg_tag)
{
    if (msg_tag.u != 1 || msg_tag.t != 0)
    {
        make_ipc_error(EINVAL);
        return;
    }
    int fd;
    L4_store_mr(VFS_STAT_FD, (unsigned *)&fd);
    struct vfs_file_state *const state = find_vfs_file_state(from, fd);
    if (state == NULL)
    {
        make_ipc_error(EINVAL);
        return;
    }
    struct romfs_file_info file_info;
    if (!romfs_file_info(&romfs_ops, state->file_off, &file_info, NULL))
    {
        make_ipc_error(EIO);
        return;
    }
    L4_load_mr(
        VFS_STAT_RET_OP,
        (L4_msg_tag_t){.u = 2, .t = 0, .flags = 0, .label = VFS_STAT_RET}.raw);
    L4_load_mr(VFS_STAT_RET_SIZE, file_info.size);
    enum vfs_file_type vfs_type = VFS_FT_OTHER;
    switch (file_info.type)
    {
    case romfs_ft_regular_file:
        vfs_type = VFS_FT_REGULAR;
        break;
    case romfs_ft_directory:
        vfs_type = VFS_FT_DIRECTORY;
        break;
    case romfs_ft_symbolic_link:
        vfs_type = VFS_FT_SYMLINK;
        break;
    case romfs_ft_hard_link:
        vfs_type = VFS_FT_HARDLINK;
        break;
    case romfs_ft_block_device:
    case romfs_ft_char_device:
    case romfs_ft_fifo:
    case romfs_ft_socket:
        vfs_type = VFS_FT_OTHER;
        break;
    }
    L4_load_mr(VFS_STAT_RET_TYPE, (unsigned)vfs_type);
    const size_t name_len = strlen(file_info.name);
    const struct L4_simple_string_item name_item = {
        .type = L4_data_type_string_item,
        .compound = 0,
        .length = name_len,
        .ptr = (unsigned)&file_info.name,
    };
    L4_load_mrs(VFS_STAT_RET_NAME, 2, (const unsigned *)&name_item);
}

static void handle_vfs_read(L4_thread_id from, L4_msg_tag_t msg_tag)
{
    if (msg_tag.u != 3 || msg_tag.t != 0)
    {
        make_ipc_error(EINVAL);
        return;
    }
    int fd;
    int offset;
    unsigned size;
    L4_store_mr(VFS_READ_FD, (unsigned *)&fd);
    L4_store_mr(VFS_READ_OFFSET, (unsigned *)&offset);
    L4_store_mr(VFS_READ_SIZE, (unsigned *)&size);
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

    L4_load_mr(
        VFS_READ_RET_OP,
        (L4_msg_tag_t){.label = VFS_READ_RET, .flags = 0, .u = 0, .t = 2}.raw);
    const struct L4_simple_string_item content_string = {
        .c = 0,
        .type = L4_data_type_string_item,
        .compound = 0,
        .length = size,
        .ptr = (unsigned)(rootfs_base + content_offset + offset)};
    L4_load_mrs(VFS_READ_RET_CONTENT, 2, (unsigned *)&content_string);
}

static void handle_vfs_readdir(L4_thread_id from, L4_msg_tag_t msg_tag)
{
    if (msg_tag.u != 2 || msg_tag.t != 0)
    {
        make_ipc_error(EINVAL);
        return;
    }
    int fd;
    size_t offset;
    L4_store_mr(VFS_READDIR_FD, (unsigned *)&fd);
    L4_store_mr(VFS_READDIR_OFFSET, &offset);
    struct vfs_file_state *const file_state = find_vfs_file_state(from, fd);
    if (file_state == NULL)
    {
        make_ipc_error(EINVAL);
        return;
    }
    if (offset == 0)
    {
        // If the user put offset zero, they want the first entry. Fetch it for
        // them.
        struct romfs_file_info info;
        if (!romfs_file_info(&romfs_ops, file_state->file_off, &info, NULL))
        {
            make_ipc_error(EINVAL);
            return;
        }
        offset = info.info;
    }
    struct romfs_file_info target_info;
    if (!romfs_file_info(&romfs_ops, offset, &target_info, NULL))
    {
        make_ipc_error(ENOENT);
        return;
    }
    L4_load_mr(
        VFS_READDIR_RET_OP,
        (L4_msg_tag_t){.u = 3, .t = 2, .flags = 0, .label = VFS_READ_RET}.raw);
    L4_load_mr(VFS_READDIR_RET_TYPE, target_info.type);
    L4_load_mr(VFS_READDIR_RET_SIZE, target_info.size);
    L4_load_mr(VFS_READDIR_RET_OFF_NEXT,
               romfs_next_file(&romfs_ops, offset, NULL));
    L4_load_mrs(VFS_READDIR_RET_NAME, 2,
                (unsigned *)&(struct L4_simple_string_item){
                    .c = 0,
                    .type = L4_data_type_string_item,
                    .compound = 0,
                    .length = strlen(target_info.name),
                    .ptr = (unsigned)target_info.name});
}

static void handle_vfs_write(L4_thread_id from, L4_msg_tag_t msg_tag)
{
    if (msg_tag.u != 3 || msg_tag.t != 2)
    {
        make_ipc_error(EINVAL);
        return;
    }
    int fd;
    L4_store_mr(VFS_WRITE_FD, (unsigned *)&fd);

    struct vfs_file_state *const file_state = find_vfs_file_state(from, fd);
    if (file_state == NULL)
    {
        make_ipc_error(EBADF);
        return;
    }

    // We cannot write to ROMFS, so the result is always the same
    make_ipc_error(EROFS);
}

static void handle_vfs_map(L4_thread_id from, L4_msg_tag_t msg_tag)
{
    if (msg_tag.u != 4 || msg_tag.t != 0)
    {
        make_ipc_error(EINVAL);
        return;
    }
    int fd;
    size_t offset;
    size_t size;
    unsigned perm;
    L4_store_mr(VFS_MAP_FD, (unsigned *)&fd);
    L4_store_mr(VFS_MAP_OFFSET, &offset);
    L4_store_mr(VFS_MAP_SIZE, &size);
    L4_store_mr(VFS_MAP_PERM, &perm);
    struct vfs_file_state *const file_state = find_vfs_file_state(from, fd);
    if (file_state == NULL)
    {
        make_ipc_error(EBADF);
        return;
    }
    if (offset + size > file_state->file_info.size)
    {
        make_ipc_error(EFAULT);
        return;
    }
    if (perm & L4_writable)
    {
        make_ipc_error(EROFS);
        return;
    }
    const size_t file_addr =
        (unsigned)rootfs_base +
        romfs_file_content_offset(&romfs_ops, file_state->file_off, NULL);
    const size_t mapping_addr = file_addr & ~(unsigned)(512 - 1);
    const size_t mapping_size =
        (file_state->file_info.size + (file_addr & (512 - 1)) + 512 - 1) &
        ~(unsigned)(512 - 1);

    L4_load_mr(
        VFS_MAP_RET_OP,
        (L4_msg_tag_t){.u = 1, .t = 2, .flags = 0, .label = VFS_MAP_RET}.raw);
    L4_load_mr(VFS_MAP_RET_ADDR, file_addr);
    const struct L4_map_item map_item = L4_map_item(
        L4_fpage_add_rights(L4_fpage(mapping_addr, mapping_size), perm),
        mapping_addr);
    L4_load_mrs(VFS_MAP_RET_MAP_ITEM, 2, (unsigned *)&map_item);
}

static L4_msg_buffer_t ipc_buffers;

static void romfs_main(L4_utcb_t *utcb);
__attribute__((noreturn, naked)) void romfs_start()
{
    asm volatile("pop {r0, r1}\n\t"
                 "movs r9, r1\n\t"
                 "ldr  r2, .Lromfs_main_got\n\t"
                 "ldr  r1, [r1, r2]\n\t"
                 "bx   r1\n\t"
                 ".align 3\n"
                 ".Lromfs_main_got:\n\t"
                 ".word %c[romfs_main](GOT)\n\t"
                 :
                 : [romfs_main] ""(romfs_main)
                 : "r0", "r1", "r2");
}

__attribute__((noreturn)) static void romfs_main(L4_utcb_t *utcb)
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
        L4_load_brs(0, 3, ipc_buffers.raw);

        const L4_msg_tag_t msg_tag =
            L4_ipc(L4_NILTHREAD, L4_ANYTHREAD, 0, &from);
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
        case VFS_STAT:
            handle_vfs_stat(from, msg_tag);
            break;
        case VFS_READ:
            handle_vfs_read(from, msg_tag);
            break;
        case VFS_READDIR:
            handle_vfs_readdir(from, msg_tag);
            break;
        case VFS_WRITE:
            handle_vfs_write(from, msg_tag);
            break;
        case VFS_MAP:
            handle_vfs_map(from, msg_tag);
            break;
        }

        const L4_msg_tag_t answer_tag = L4_ipc(
            from, L4_NILTHREAD, L4_timeouts(L4_zero_time, L4_zero_time), &from);
        if (L4_ipc_failed(answer_tag))
        {
            // TODO: Log error about IPC error
            continue;
        }
    }
    // In case we reach the end of this program, delete us
    delete_romfs_thread();
}
