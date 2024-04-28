cmake_minimum_required(VERSION 3.20)

# Right now we are building the complete kernel twice.
# This is because the kernel for flash storage requires relocatiosn
# and needs to be compiled with some indirections, while the kernel
# for SRAM is statically linked and should be as small as anyhow
# possible.

set(KERN_RAM_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/pico_ram.ld)
set(KERN_FLASH_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/pico_flash.ld)

add_executable(
  kern

  start.s
)
target_link_libraries(
  kern
  PUBLIC
  libkern
)
target_compile_options(
  kern
  PUBLIC
  -fPIC
  -msingle-pic-base
)
target_link_options(
  kern
  PUBLIC
  -pie
  -Wl,-E,-z,text
  -T ${KERN_FLASH_LINKER_SCRIPT}
)
set_target_properties(
  kern
  PROPERTIES
  OUTPUT_NAME kern
  LINK_DEPENDS ${KERN_FLASH_LINKER_SCRIPT}
)

add_executable(
  kern_ram

  start.s
)
target_link_libraries(
  kern_ram
  PUBLIC
  libkern
)
target_link_options(
  kern_ram
  PUBLIC
  -T ${KERN_RAM_LINKER_SCRIPT}
)
set_target_properties(
  kern_ram
  PROPERTIES
  LINK_DEPENDS ${KERN_RAM_LINKER_SCRIPT}
)
