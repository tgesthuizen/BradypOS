#include "bootinfo.h"
#include "string.h"

#include <libelf.h>
#include <romfs.h>

#include <stddef.h>
#include <stdint.h>

extern unsigned char __stack_top[];
int main();
void _start();
static __attribute__((naked)) void isr_invalid()
{
    __asm__ volatile("bkpt #0\n\t"
                     "bx   lr\n\t");
}
static __attribute__((naked)) void unhandled_irq()
{
    __asm__ volatile("mrs r0, ipsr\n\t"
                     "sub r0, #16\n\t"
                     ".global unhandled_user_irq_num_in_r0\n"
                     "unhandled_user_irq_num_in_r0:\n\t"
                     "bkpt #0\n\t"
                     "bx lr\n\t");
}

__attribute__((section(".vector"))) const uintptr_t __vector[] = {
    (uintptr_t)__stack_top,   (uintptr_t)_start,
    (uintptr_t)isr_invalid,   (uintptr_t)isr_invalid,
    (uintptr_t)isr_invalid,   (uintptr_t)isr_invalid,
    (uintptr_t)isr_invalid,   (uintptr_t)isr_invalid,
    (uintptr_t)isr_invalid,   (uintptr_t)isr_invalid,
    (uintptr_t)isr_invalid,   (uintptr_t)isr_invalid,
    (uintptr_t)isr_invalid,   (uintptr_t)isr_invalid,
    (uintptr_t)isr_invalid,   (uintptr_t)isr_invalid,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq};

__attribute__((naked)) void _start()
{
    asm volatile("movs r0, #0\n\t"
                 "movs lr, r0\n\t"
                 "ldr  r0, =__bss_start\n\t"
                 "movs r1, #0\n\t"
                 "ldr  r2, =__bss_len\n\t"
                 "bl   memset\n\t"
                 "ldr  r0, =__data_start\n\t"
                 "ldr  r1, =__data_lma\n\t"
                 "ldr  r2, =__data_len\n\t"
                 "bl   memcpy\n\t"
                 "bl   main\n\t"
                 "b    ." ::
                     : "r0", "r1", "r2", "lr");
}

extern unsigned char __romfs_start[];

static int romfs_rom_map(void **data, size_t offset, size_t size, void *user)
{
    (void)size;
    (void)user;
    *data = __romfs_start + offset;
    return 0;
}

static void romfs_rom_unmap(void **data, size_t size, void *user)
{
    (void)data;
    (void)size;
    (void)user;
}

static const struct romfs_block_iface romfs_rom_mapping = {
    .map = romfs_rom_map, .unmap = romfs_rom_unmap};

static bool memcmp(const char *lhs, const char *rhs, size_t len)
{
    while (len--)
    {
        if (*lhs++ != *rhs++)
            return false;
    }
    return true;
}

struct elf_file_state
{
    unsigned char *elf_file_base;
    unsigned char *sram_marker;
};

static int elf_read_file(size_t offset, size_t size, void *data, void *user)
{
    struct elf_file_state *const state = user;
    unsigned char *ucdata = data;
    for (size_t i = 0; i < size; ++i)
    {
        ucdata[i] = state->elf_file_base[offset + i];
    }
    return 0;
}

// Assume align is a power of two
static unsigned char *align_ptr(unsigned char *ptr, unsigned align)
{
    return (unsigned char *)(((uintptr_t)ptr + (align - 1)) & ~(align - 1));
}

static int elf_alloc_rw(void **location, size_t size, size_t align, int perm,
                        void *user)
{
    (void)perm;
    struct elf_file_state *state = user;

    state->sram_marker = align_ptr(state->sram_marker, align);
    *location = state->sram_marker;
    state->sram_marker += size;
    return 0;
}

static int elf_map(void **location, size_t offset, size_t size, size_t align,
                   int perm, void *user)
{
    (void)size;
    (void)align;
    (void)perm;
    struct elf_file_state *state = user;

    *location = state->elf_file_base + offset;
    return 0;
}

static const struct libelf_ops elf_ops = {
    .read_elf_file = elf_read_file,
    .alloc_rw = elf_alloc_rw,
    .map = elf_map,
    .symbol = NULL,
};

extern unsigned char __sram_start;

int main()
{
    struct romfs_info info_block;
    if (!romfs_info(&romfs_rom_mapping, NULL, &info_block))
    {
        // TODO: Use a panic function here
        return 1;
    }

    static const char sbin_dir_name[] = "sbin";
    const size_t first_file = romfs_root_directory(&romfs_rom_mapping, NULL);
    struct romfs_file_info file_info;
    size_t sbin_dir = ROMFS_INVALID_FILE;
    for (size_t current_file = first_file; current_file != ROMFS_INVALID_FILE;
         current_file = romfs_next_file(&romfs_rom_mapping, current_file, NULL))
    {
        if (!romfs_file_info(&romfs_rom_mapping, current_file, &file_info,
                             NULL))
            continue;
        // This is safe: The buffer for the filename is at least 16 bytes long
        if (memcmp(file_info.name, sbin_dir_name, sizeof(sbin_dir_name)))
        {
            sbin_dir = current_file;
            break;
        }
    }
    size_t kern_file = romfs_openat(&romfs_rom_mapping, sbin_dir, "kern", NULL);
    size_t root_file = romfs_openat(&romfs_rom_mapping, sbin_dir, "root", NULL);
    unsigned char *const kern_elf_base =
        __romfs_start +
        romfs_file_content_offset(&romfs_rom_mapping, kern_file, NULL);
    unsigned char *const root_elf_base =
        __romfs_start +
        romfs_file_content_offset(&romfs_rom_mapping, root_file, NULL);

    struct elf_file_state file_state = {
        .elf_file_base = kern_elf_base,
        .sram_marker = &__sram_start,
    };
    static struct libelf_state kern_state;
    static struct libelf_loaded_segment kern_segs[4];
    kern_state.max_segments = 4;
    kern_state.segments = kern_segs;
    static struct libelf_state root_state;
    static struct libelf_loaded_segment root_segs[4];
    root_state.max_segments = 4;
    root_state.segments = root_segs;

    if (load_elf_file(&elf_ops, &kern_state, &file_state) != LIBELF_OK)
        return 1;

    static const char got_symbol_name[] = "__global_offset_table";

    unsigned kern_got = 0;
    if (locate_elf_symbol(&kern_state, got_symbol_name, &kern_got) != LIBELF_OK)
    {
        return 1;
    }

    file_state.elf_file_base = root_elf_base;
    if (load_elf_file(&elf_ops, &root_state, &file_state) != LIBELF_OK)
        return 1;

    unsigned root_got = 0;
    if (locate_elf_symbol(&root_state, got_symbol_name, &root_got) != LIBELF_OK)
    {
        return 1;
    }

    construct_boot_info(&kern_state, &root_state, file_state.sram_marker);

    asm volatile("gdb_intercept_elf_positions_here:\n\t");
    register void *mem_info asm("r0") = file_state.sram_marker;
    register unsigned kern_got_reg asm("r9") = kern_got;
    asm("blx %[entry_point]"
        :
        : [entry_point] "r"(kern_state.entry_point), "r"(mem_info),
          "r"(kern_got_reg)
        : "memory", "cc");
    return 0;
}
