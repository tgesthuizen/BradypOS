cmake_minimum_required(VERSION 3.20)

project(BradypOS
  LANGUAGES C ASM
  VERSION 0.0.1
)

add_compile_options(
  -mcpu=cortex-m0plus
  -Wall
  -Wextra
)

add_subdirectory(kern)
