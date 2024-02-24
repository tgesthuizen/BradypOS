cmake_minimum_required(VERSION 3.20)

add_executable(
  kern

  debug.c
  main.c
  semihosting.c
  start.s
  string.c
  vector.c
)

target_include_directories(
  kern
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIRECTORY}
)

set(KERN_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/pico_ram.ld)

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
