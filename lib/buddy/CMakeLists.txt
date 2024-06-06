cmake_minimum_required(VERSION 3.20)

project(
  libbuddy
  LANGUAGES C
  VERSION 0.0.1
)

add_library(
  libbuddy
  STATIC
  buddy.c
  linked_list_alloc.c
)
target_include_directories(
  libbuddy
  PUBLIC
  include
)
set_target_properties(
  libbuddy
  PROPERTIES
  OUTPUT_NAME buddy
)
