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

set(KERN_LINKER_SCRIPT "${BRADYPOS_ROM_APP_LINKER_SCRIPT}")
set(KERN_VERSION_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/version.ld")

add_executable(
  kern
  debug.c
  interrupts.c
  kalarm.c
  kip.s
  main.c
  memory.c
  platform.c
  softirq.c
  start.s
  syscall.c
  syscall/ipc.c
  syscall/schedule.c
  syscall/space_control.c
  syscall/thread_control.c
  syscall/unmap.c
  systhread.c
  systick.c
  thread.c
  vector.c
)

target_link_libraries(
  kern
  PUBLIC
  heap
  kernel_headers
  l4
  libstring
  semihosting
  tinyprintf
)
target_compile_options(
  kern
  PRIVATE
  -fvisibility=hidden
)
target_compile_options(
  kern
  PUBLIC
  -fPIE
  -msingle-pic-base
  -mno-pic-data-is-text-relative
)
target_link_options(
  kern
  PUBLIC
  -pie
  -Wl,-E,-z,text,-z,common-page-size=512,-z,max-page-size=512
  -T ${KERN_LINKER_SCRIPT}
  -Wl,--version-script,${KERN_VERSION_SCRIPT}
)
set_target_properties(
  kern
  PROPERTIES
  OUTPUT_NAME kern
  LINK_DEPENDS "${KERN_LINKER_SCRIPT};${KERN_VERSION_SCRIPT}"
)


add_subdirectory(ssp)
