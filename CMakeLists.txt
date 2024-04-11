cmake_minimum_required(VERSION 3.10)

project(libelf
  LANGUAGES C
)

add_library(
  libelf
  STATIC
  elf.c
)
target_include_directories(
  libelf
  PUBLIC
  include
)
