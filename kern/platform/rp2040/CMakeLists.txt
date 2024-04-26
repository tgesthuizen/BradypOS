cmake_minimum_required(VERSION 3.20)

set(KERN_RAM_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/pico_ram.ld)
set(KERN_FLASH_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/pico_flash.ld)

target_sources(
  kern
  PUBLIC
  start.s
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
  -Wl,-z,text
)

target_link_options(
  kern
  PUBLIC
  -T ${KERN_FLASH_LINKER_SCRIPT}
)
set_target_properties(
  kern
  PROPERTIES
  OUTPUT_NAME kern
  LINK_DEPENDS ${KERN_FLASH_LINKER_SCRIPT}
)
