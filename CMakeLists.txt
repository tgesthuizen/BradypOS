cmake_minimum_required(VERSION 3.20)

project(BradypOS
  LANGUAGES C ASM
  VERSION 0.0.1
)

add_compile_options(
  -mcpu=cortex-m0plus
  -Wall
  -Wextra
)

add_executable(
  kern

  main.c
  start.s
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
