cmake_minimum_required(VERSION 3.20)

project(
  librp2040_utils
  LANGUAGES C
  VERSION 0.1.0
)

add_library(
  librp2040_utils
  STATIC
  resets.c
)
target_include_directories(
  librp2040_utils
  PUBLIC
  include
)
set_target_properties(
  librp2040_utils
  PROPERTIES
  OUTPUT_NAME rp2040_utils
)
