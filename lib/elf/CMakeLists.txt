cmake_minimum_required(VERSION 3.10)

project(libelf
  LANGUAGES C
)

add_library(
  libelf${LIBELF_TARGET_POSTFIX}
  STATIC
  elf.c
)
target_compile_options(
  libelf${LIBELF_TARGET_POSTFIX}
  PRIVATE
  -ffreestanding
)
target_include_directories(
  libelf${LIBELF_TARGET_POSTFIX}
  PUBLIC
  include
)
set_target_properties(
  libelf${LIBELF_TARGET_POSTFIX}
  PROPERTIES
  OUTPUT_NAME elf
)

option(LIBELF_BUILD_DEMOS "Whether to build libelf demo applications" OFF)
if(LIBELF_BUILD_DEMOS)
  add_subdirectory(demos)
endif()
