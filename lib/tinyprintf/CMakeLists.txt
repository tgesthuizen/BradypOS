cmake_minimum_required(VERSION 3.10)

project(tinyprintf
  LANGUAGES C
)

enable_testing()

add_library(
  tinyprintf
  STATIC
  tinyprintf.c
)
target_include_directories(
  tinyprintf
  PUBLIC
  .
)

option(TINYPRINTF_ENABLE_TESTS "Whether to build tests for tinyprintf" OFF)

if(TINYPRINTF_ENABLE_TESTS)
  add_executable(printf-test test/printf.c)
  target_link_libraries(printf-test PUBLIC tinyprintf)
  add_test(printf printf-test)
  add_executable(sprintf-test test/sprintf.c)
  target_link_libraries(sprintf-test PUBLIC tinyprintf)
  add_test(sprintf sprintf-test)
endif()
