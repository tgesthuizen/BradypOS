cmake_minimum_required(VERSION 3.20)

project(heap
  LANGUAGES C
  VERSION 0.1.0
)

add_library(heap INTERFACE)
target_include_directories(heap INTERFACE include)

option(HEAP_TESTS "Whether to build tests for libheap" OFF)
if(HEAP_TESTS)
  enable_testing()
  add_executable(heap-tests test.c)
  target_link_libraries(heap-tests PUBLIC heap)
  add_test(NAME heap COMMAND heap-tests)
endif()
