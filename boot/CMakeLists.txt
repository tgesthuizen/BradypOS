cmake_minimum_required(VERSION 3.20)

project(boot
  LANGUAGES C ASM
  VERSION 0.0.1
)

add_executable(boot
  boot.c
)
target_compile_options(boot
  PUBLIC
  -ffreestanding
)
