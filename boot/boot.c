#include "romfs.h"
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

void *memset(void *ptr, int value, size_t num)
{
    unsigned char *cptr = (unsigned char *)ptr;
    while (num--)
    {
        *cptr++ = value;
    }
    return ptr;
}

void *memcpy(void *dest, const void *src, size_t size)
{
    unsigned char *cdest = (unsigned char *)dest;
    const unsigned char *csrc = (const unsigned char *)src;
    while (size--)
    {
        *cdest++ = *csrc++;
    }
    return dest;
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
    const unsigned char *const kern_elf_base =
        __romfs_start +
        romfs_file_content_offset(&romfs_rom_mapping, kern_file, NULL);
    const unsigned char *const root_elf_base =
        __romfs_start +
        romfs_file_content_offset(&romfs_rom_mapping, root_file, NULL);

    return 0;
}
