cmake_minimum_required(VERSION 3.20)

add_library(
  libkern
  INTERFACE
)

target_sources(
  libkern
  INTERFACE
  debug.c
  main.c
  semihosting.c
  string.c
  vector.c
)

target_include_directories(
  libkern
  INTERFACE
  include
)
target_compile_options(
  libkern
  INTERFACE
  -fvisibility=hidden
)

add_subdirectory(platform/${BRADYPOS_TARGET_PLATFORM})
add_subdirectory(ssp)
