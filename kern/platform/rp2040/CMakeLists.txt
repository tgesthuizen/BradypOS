cmake_minimum_required(VERSION 3.20)

set(KERN_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/pico_ram.ld)

target_sources(
  kern
  PUBLIC
  start.s
)

target_link_options(
  kern
  PUBLIC
  -T ${KERN_LINKER_SCRIPT}
)
set_target_properties(
  kern
  PROPERTIES
  OUTPUT_NAME kern.elf
  LINK_DEPENDS ${KERN_LINKER_SCRIPT}
)
