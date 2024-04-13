cmake_minimum_required(VERSION 3.20)

project(libromfs
  LANGUAGES C
  VERSION 0.0.1
)

add_library(libromfs romfs.c)
target_include_directories(libromfs PUBLIC include)
target_compile_options(libromfs PRIVATE -ffreestanding)

