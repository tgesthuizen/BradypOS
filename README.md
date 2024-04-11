# libelf

`libelf` is a library for loading ELF files in the BradypOS operating
system. In order to be usable in different circumstances, including
userspace and the bootloader, the library does not rely on external
memory allocation nor file access.
In order to use it, the user provides a dynamic interface for
accessing the ELF file and mapping segments.

A fair warning: `libelf` is not a generic ELF loader.
It will only load ELF executables for the BradypOS operating system.
Loading anything else should be considered experimental.
