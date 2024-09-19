cmake_minimum_required(VERSION 3.20)

project(pl011
  LANGUAGES C ASM
  VERSION 0.1.0
)

add_executable(
  pl011

  data.s
  pl011.c
)

target_link_libraries(
  pl011
  PUBLIC
  l4
  libroot
  libstring
)
