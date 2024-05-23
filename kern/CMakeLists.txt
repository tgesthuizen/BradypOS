cmake_minimum_required(VERSION 3.20)

# Make the kernel headers a separate library.
# This allows for kernel "plugins" like the SSP support.
add_library(
  kernel_headers
  INTERFACE
)
target_include_directories(
  kernel_headers
  INTERFACE
  include
)

add_library(
  libkern
  INTERFACE
)

target_sources(
  libkern
  INTERFACE
  debug.c
  interrupts.c
  kalarm.c
  kip.s
  main.c
  memory.c
  platform.c
  softirq.c
  syscall.c
  systhread.c
  systick.c
  thread.c
  vector.c
)
target_link_libraries(
  libkern
  INTERFACE
  heap
  kernel_headers
  l4
  libstring
  semihosting
  tinyprintf
)
target_compile_options(
  libkern
  INTERFACE
  -fvisibility=hidden
)

add_subdirectory(platform/${BRADYPOS_TARGET_PLATFORM})
add_subdirectory(ssp)
