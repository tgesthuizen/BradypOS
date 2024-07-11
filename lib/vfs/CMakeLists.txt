cmake_minimum_required(VERSION 3.20)

project(libvfs
  LANGUAGES C
  VERSION 0.1.0
)

add_library(
  libvfs
  STATIC
  vfs-client.c
)

target_include_directories(libvfs PUBLIC include)
target_link_libraries(libvfs PRIVATE l4)
target_compile_options(libvfs PRIVATE -ffunction-sections)
set_target_properties(
  libvfs
  PROPERTIES
  OUTPUT_NAME vfs
)
