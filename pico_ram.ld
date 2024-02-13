OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)

MEMORY
{
    FLASH(rx) : ORIGIN = 0x10000000, LENGTH = 2048k
    RAM(rwx) : ORIGIN =  0x20000000, LENGTH = 256k
    SCRATCH_X(rwx) : ORIGIN = 0x20040000, LENGTH = 4k
    SCRATCH_Y(rwx) : ORIGIN = 0x20041000, LENGTH = 4k
}

PHDRS
{
    rox      PT_LOAD FILEHDR PHDRS;
    rw       PT_LOAD;
    scratch  PT_LOAD;
}

ENTRY(_start)

SECTIONS
{
    . = SIZEOF_HEADERS;
    .text : {
      *(.text)
      *(.text.*)
    } >RAM :rox
    .rodata : {
      *(.rodata)
      *(.rodata.*)
    } >RAM :rox
    
    .data : {
      *(.data)
      *(.data.*)
    } >RAM :rw
    .bss (NOLOAD) : {
      __bss_start = .;
      *(.bss)
      *(.bss.*)
      __bss_end  = .;
    } >RAM :rw
    
    .stack (NOLOAD) : {
      __stack = ALIGN(0x10);
      . = . + 0x200;
    } >SCRATCH_X :scratch = 0x00
}

