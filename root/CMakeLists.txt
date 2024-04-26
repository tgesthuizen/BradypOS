cmake_minimum_required(VERSION 3.20)

project(
  root
  LANGUAGES C
  VERSION 0.0.1
)

add_executable(root root.c)
target_compile_options(
  root
  PUBLIC
  -ffreestanding
  -fPIE
  -msingle-pic-base
  -fvisibility=hidden
)
target_link_options(
  root
  PUBLIC
  -Wl,-E
  -T ${CMAKE_CURRENT_SOURCE_DIR}/root.ld
  -pie
)
set_target_properties(
  root
  PROPERTIES
  OUTPUT_NAME root
  LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/root.ld
)
