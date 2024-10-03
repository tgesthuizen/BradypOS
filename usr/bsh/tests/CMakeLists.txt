cmake_minimum_required(VERSION 3.20)

project(bsh-tests
  LANGUAGES C
  VERSION 0.1
)

enable_testing()

set(
  PARSER_TEST_CASES
  basic_command
  incomplete_escape_sequence
)

add_executable(
  parser_driver
  ../parser.c
  parser_driver.c
)

foreach(TEST_CASE ${PARSER_TEST_CASES})
  set(TEST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/parser_tests/${TEST_CASE}.txt")
  add_test(
    NAME ${TEST_CASE}
    COMMAND bash -c "sed '/---/Q' ${TEST_FILE} | $<TARGET_FILE:parser_driver> | diff - <( sed '1,/---/ d' ${TEST_FILE} )"
  )
endforeach()
