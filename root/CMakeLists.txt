cmake_minimum_required(VERSION 3.20)

project(
  root
  LANGUAGES C
  VERSION 0.0.1
)

add_executable(
  root
  data.s
  root.c
)
target_compile_options(
  root
  PUBLIC
  -fvisibility=hidden
)
target_link_libraries(
  root
  PUBLIC
  l4
  semihosting
)
target_link_options(
  root
  PUBLIC
  -Wl,-E,-z,text,-z,common-page-size=512,-z,max-page-size=512
  -T ${CMAKE_CURRENT_SOURCE_DIR}/root.ld
  -pie
)
set_target_properties(
  root
  PROPERTIES
  OUTPUT_NAME root
  LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/root.ld
)
