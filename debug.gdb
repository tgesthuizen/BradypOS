# TODO: Parameterize this with the actual build

source lib/elf/elf-sniffer.py
monitor arm semihosting enable

define launch_into_kernel
  delete breakpoints
  file
  file build/boot/boot.elf
  break gdb_intercept_elf_positions_here
  commands $bpnum
    add-symbol-file-for-pie build/kern/platform/rp2040/kern kern_state
    add-symbol-file-for-pie build/root/root root_state
    cont
  end
  enable once $bpnum
  monitor reset init
  cont
end

define reflash
  dont-repeat
  monitor program build/dist/bradypos.bin 0x10000000 verify
  launch_into_kernel
end
