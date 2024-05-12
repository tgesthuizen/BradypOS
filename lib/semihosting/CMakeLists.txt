add_library(
  semihosting
  STATIC
  semihosting.c
)

target_compile_options(
  semihosting
  PRIVATE
  -ffunction-sections
  -ffreestanding
)

target_include_directories(
  semihosting
  PUBLIC
  include
)
target_link_libraries(
  semihosting
  PRIVATE
  libstring
)
