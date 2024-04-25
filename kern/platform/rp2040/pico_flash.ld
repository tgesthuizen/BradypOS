OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)

PHDRS
{
    headers  PT_PHDR PHDRS;
    rox      PT_LOAD FILEHDR PHDRS;
    rw       PT_LOAD;
    scratch  PT_LOAD;
    dynamic  PT_DYNAMIC;
}

ENTRY(_start)

SECTIONS
{
    . = SIZEOF_HEADERS;
    .text : {
      *(.text)
      *(.text.*)
    } :rox
    .rodata : {
      *(.rodata)
      *(.rodata.*)
    } :rox

    .dynamic : {
      *(.dynamic)
    } :rw :dynamic
    .got : {
      __global_offset_table = .;
      *(.got)
      *(.got*)
    } :rw
    .data : {
      *(.data)
      *(.data.*)
    } :rw
    .bss (NOLOAD) : {
      __bss_start = .;
      *(.bss)
      *(.bss.*)
      __bss_end = .;
    } :rw
}
