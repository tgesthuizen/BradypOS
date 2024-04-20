cmake_minimum_required(VERSION 3.10)

project(libelf
  LANGUAGES C
)

add_library(
  libelf
  STATIC
  elf.c
)
target_compile_options(
  libelf
  PRIVATE
  -ffreestanding
)
target_include_directories(
  libelf
  PUBLIC
  include
)
set_target_properties(
  libelf
  PROPERTIES
  OUTPUT_NAME elf
)
