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

## Implementation Details

`libelf` implements some optimizations which are required for the
efficient operation of BradypOS.
Most notably, it implements an optimization to load ELF executables
from the internal ROM of the microcontroller.
If a read-only, possibly executable, ELF segment is loaded from a ROM
section and is properly aligned, the loader will simply leave it in
ROM and reference it accordingly.
Loading in this mode will fail if the loader encounters any
relocations that have to be applied to one of the segments in ROM.
Furthermore, the loader can support to either load executables to a
random place in memory or to fixed locations.
