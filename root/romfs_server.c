#include "variables.h"
#include <l4/bootinfo.h>
#include <l4/ipc.h>
#include <l4/kip.h>
#include <l4/schedule.h>
#include <l4/space.h>
#include <l4/thread.h>
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

__attribute__((noreturn, naked)) void romfs_start()
{
    asm("pop {r0, r1}\n\t"
        "movs r9, r1\n\t"
        "b romfs_main\n\t");
}

static char filename_buf[VFS_PATH_MAX + 1];

__attribute__((noreturn)) void romfs_main(L4_utcb_t *utcb)
{
    (void)utcb;
    locate_romfs_start(the_kip);
    L4_msg_buffer_t ipc_buffers;
    const L4_acceptor_t ipc_acceptor = L4_string_items_acceptor;
    ipc_buffers.raw[0] = ipc_acceptor.raw;
    const struct L4_simple_string_item filename_buf_item = {
        .length = VFS_PATH_MAX + 1,
        .c = 0,
        .type = 0,
        .compound = 0,
        .ptr = (unsigned)&filename_buf};
    memcpy(ipc_buffers.raw + 1, &filename_buf_item, sizeof(unsigned) * 2);
    while (1)
    {
        L4_thread_id from;
        L4_msg_tag_t tag;
        L4_load_brs(0, 3, ipc_buffers.raw);

        tag = L4_ipc(L4_NILTHREAD, L4_ANYTHREAD, 0, &from);
        if (L4_ipc_failed(tag))
        {
            // TODO: Log error about IPC error
            continue;
        }
        //switch (tag.label)
        //{
        //case VFS_OPENAT:
	//
        //}
    }
    // In case we reach the end of this program, delete us
    delete_romfs_thread();
}
