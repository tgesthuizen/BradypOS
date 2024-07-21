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
        KEEP(*(.boot2))
    } >FLASH
    .vector : {
        KEEP(*(.vector))
    } >FLASH

    # Normal linking starts here
    .text : {
        *(.text)
	*(.text.*)
    } >FLASH
    .data : {
    	PROVIDE(__sram_start = .);
    	. += 0x8000; # Give some headroom for kernel and root segments in SRAM
        PROVIDE(__data_start = .);
        *(.data)
	*(.data.*)
	PROVIDE(__data_end = .);
    } >RAM AT>FLASH
    PROVIDE(__data_len = __data_end - __data_start);
    PROVIDE(__data_lma = .);

    .bss (NOLOAD) : {
    	PROVIDE(__bss_start = .);
        *(.bss)
	*(.bss.*)
	PROVIDE(__bss_end = .);
    } >RAM
    PROVIDE(__bss_len = __bss_end - __bss_start);

    .stack (NOLOAD) : {
      PROVIDE(__stack = ALIGN(0x10));
      . = . + 0x200;
      PROVIDE(__stack_top = .);
    } >SCRATCH_X

    .romfs : {
      PROVIDE(__romfs_start = ALIGN(1024));
    } >FLASH
}
