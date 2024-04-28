cmake_minimum_required(VERSION 3.20)

# Make the kernel headers a separate library.
# This allows for kernel "plugins" like the SSP support.
add_library(
  kernel_headers
  INTERFACE
)
target_include_directories(
  kernel_headers
  INTERFACE
  include
)

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
target_link_libraries(
  libkern
  INTERFACE
  kernel_headers
)
target_compile_options(
  libkern
  INTERFACE
  -fvisibility=hidden
)

add_subdirectory(platform/${BRADYPOS_TARGET_PLATFORM})
add_subdirectory(ssp)
