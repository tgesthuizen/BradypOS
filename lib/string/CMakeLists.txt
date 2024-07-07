project(
  libstring
  LANGUAGES C
  VERSION 0.1.0
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
  # Forbid LTO
  # This prohibits a bug in GCCs optimizer, recognizing the structure
  # of memset and generating a call to it - when inlining memset.
  # This generates weird 'undefined reference to memset' errors.
  -fno-lto
)
