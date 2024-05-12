project(
  libstring
  LANGUAGES C
  VERSION 0.0.1
)

add_library(
  libstring${LIBSTRING_TARGET_POSTFIX}
  STATIC
  string.c
)
target_include_directories(
  libstring${LIBSTRING_TARGET_POSTFIX}
  PUBLIC
  include
)
target_compile_options(
  libstring${LIBSTRING_TARGET_POSTFIX}
  PRIVATE
  -ffreestanding
)
