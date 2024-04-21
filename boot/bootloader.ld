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
        __data_start = .;
        *(.data)
	*(.data.*)
	__data_end = .;
    } >RAM AT>FLASH
    __data_len = __data_end - __data_start;
    __data_lma = .;

    .bss (NOLOAD) : {
    	__bss_start = .;
        *(.bss)
	*(.bss.*)
	__bss_end = .;
    } >RAM
    __bss_len = __bss_end - __bss_start;

    .stack (NOLOAD) : {
      __stack = ALIGN(0x10);
      . = . + 0x200;
      __stack_top = .;
    } >SCRATCH_X

    .romfs : {
      __romfs_start = ALIGN(1024);
    } >FLASH
}
