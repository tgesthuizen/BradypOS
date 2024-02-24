cmake_minimum_required(VERSION 3.20)

add_library(kern_common INTERFACE)
target_include_directories(kern_common INTERFACE include)

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
target_link_libraries(kern PUBLIC kern_common)

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

add_subdirectory(ssp)
