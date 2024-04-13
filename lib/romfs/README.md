# libromfs

libromfs is BradypOS's implementation of a library that reads ROMFS
filesystems.
They are used in the RP2040's internal ROM and contain the read-only
contents of the core system.
Because this library is both utilized by the boot loader as well as
the userspace driver for the file system, it needs to compiled for
freestanding systems, yet work in a userspace scenario.
Therefore, it offers a transparent interface for block-device access.
In order to allow for the exec-from-ROM hack that the ELF loader
implements for resource-efficient ELF loading, it also contains a
method to obtain the offset of a structure in the file system.

