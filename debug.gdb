# TODO: Parameterize this with the actual build

source lib/elf/elf-sniffer.py
monitor arm semihosting enable

define launch_into_kernel
  delete breakpoints
  file
  file build/boot/boot.elf
  break gdb_intercept_elf_positions_here
  commands $bpnum
    add-symbol-file-for-pie build/kern/kern kern_state
    add-symbol-file-for-pie build/usr/root/root root_state
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

define thread_cont
  set $tcb = find_thread_by_global_id($arg0)
  if $tcb == NULL
    print "Couldn't find a thread with that id"
    return
  end
  set $thread_pc = (unsigned *)(((unsigned *)$tcb->ctx.sp)[THREAD_CTX_STACK_PC])
  break *$thread_pc
  enable once $bpnum
  continue
end
