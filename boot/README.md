# boot

This is `boot`, the bootloader of the BradypOS operating system.
As the kernel is meant to be written without being tied to a specific
system configuration, the system requires a bootloader that configures
the root task and kernel.
This way the system can boot without a filesystem driver and
configuration management in the kernel.

## Boot Procedure

`boot` does a sequence of actions in order to get the kernel started:
- Start executing the boot2 stage, adapted from the Raspberry Pi Pico
  SDK. The payload maps the flash into memory and executes the actual
  bootloader.
- The actual bootloader runs from flash and starts reading the ROMFS
  filesystem that follows the location of its own executable.
- It loads the ELF executables `/sbin/kern` and `/sbin/root`.
  This is target to the ROM loading optimization explained in libelf.
- Next, it writes the boot info for the kernel.
  The boot info consists of a variety of informations that the kernel
  needs to successfully start and run the root process.
  - The SRAM chunks occupied by the kernel and the root process.
  - The location of the root process's UTCB.
  - The initial IP and SP of the root process.
- It jumps to the kernel, passing the boot info.
