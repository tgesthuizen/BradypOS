cmake_minimum_required(VERSION 3.20)

project(
  root
  LANGUAGES C
  VERSION 0.0.1
)

add_executable(root root.c)
target_compile_options(
  root
  PUBLIC
  -ffreestanding
)
