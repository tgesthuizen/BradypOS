cmake_minimum_required(VERSION 3.20)

# Right now we are building the complete kernel twice.
# This is because the kernel for flash storage requires relocations
# and needs to be compiled with some indirections, while the kernel
# for SRAM is statically linked and should be as small as anyhow
# possible.

set(KERN_RAM_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/pico_ram.ld)
set(KERN_FLASH_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/pico_flash.ld)
set(KERN_VERSION_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/version.ld)

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
  -fPIE
  -msingle-pic-base
  -mno-pic-data-is-text-relative
)
target_compile_definitions(
  kern
  PUBLIC
  KERN_TARGET=flash
)
target_link_options(
  kern
  PUBLIC
  -pie
  -Wl,-E,-z,text,-z,common-page-size=512,-z,max-page-size=512
  -T ${KERN_FLASH_LINKER_SCRIPT}
  -Wl,--version-script,${KERN_VERSION_SCRIPT}
)
set_target_properties(
  kern
  PROPERTIES
  OUTPUT_NAME kern
  LINK_DEPENDS "${KERN_FLASH_LINKER_SCRIPT};${KERN_VERSION_SCRIPT}"
)
