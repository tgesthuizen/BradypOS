OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)

PHDRS
{
    headers  PT_PHDR PHDRS;
    rox      PT_LOAD FILEHDR PHDRS;
    rw       PT_LOAD;
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

    . = ALIGN(CONSTANT(COMMONPAGESIZE));
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
      PROVIDE(__bss_start = .);
      *(.bss)
      *(.bss.*)
      PROVIDE(__bss_end = .);
    } :rw
}
