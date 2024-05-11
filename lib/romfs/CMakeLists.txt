cmake_minimum_required(VERSION 3.20)

project(libromfs
  LANGUAGES C
  VERSION 0.0.1
)

add_library(
  libromfs${LIBROMFS_TARGET_POSTFIX}
  STATIC
  romfs.c
)
target_include_directories(libromfs${LIBROMFS_TARGET_POSTFIX} PUBLIC include)
target_compile_options(libromfs${LIBROMFS_TARGET_POSTFIX} PRIVATE -ffreestanding)
set_target_properties(
  libromfs${LIBROMFS_TARGET_POSTFIX}
  PROPERTIES
  OUTPUT_NAME romfs
)

option(
  LIBROMFS_BUILD_DEMOS
  "Whether to build demo applications of libromfs (only for POSIX platforms)"
  OFF
)
if(LIBROMFS_BUILD_DEMOS)
  add_subdirectory(demos)
endif()
