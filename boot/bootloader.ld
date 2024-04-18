OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)

MEMORY
{
    FLASH(rx) : ORIGIN = 0x10000000, LENGTH = 2048k
    RAM(rwx) : ORIGIN =  0x20000000, LENGTH = 256k
    SCRATCH_X(rwx) : ORIGIN = 0x20040000, LENGTH = 4k
    SCRATCH_Y(rwx) : ORIGIN = 0x20041000, LENGTH = 4k
}

ENTRY(_start)

SECTIONS
{
    # The executable is required to start with these sections!
    .boot2 : {
        *(.boot2)
    } >FLASH
    .vector : {
        *(.vector)
    } >FLASH

    # Normal linking starts here
    .text : {
        *(.text)
	*(.text.*)
    } >FLASH
    .data : {
        *(.data)
	*(.data.*)
    } >RAM AT>FLASH
    .bss (NOLOAD) : {
        *(.bss)
	*(.bss.*)
    } >RAM

    .stack (NOLOAD) : {
      __stack = ALIGN(0x10);
      . = . + 0x200;
    } >SCRATCH_X

    __romfs_start = .;
}
