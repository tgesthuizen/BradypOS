cmake_minimum_required(VERSION 3.20)

project(BradypOS
  LANGUAGES C ASM
  VERSION 0.0.1
)

add_compile_options(
  -Wall
  -Wextra
)

set(BRADYPOS_TARGET_PLATFORM "rp2040" CACHE STRING "Platform to target")

include(GNUInstallDirs)

set(BRADYPOS_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
set(BRADYPOS_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
set(BRADYPOS_INSTALL_INCLUDEDIR ${CMAKE_INSTALL_INCLUDEDIR})

set(
  CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/platform/${BRADYPOS_TARGET_PLATFORM}"
)

include(Init)

add_subdirectory(boot)

add_library(l4_common INTERFACE)
target_include_directories(
  l4_common
  INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_library(pie_binary INTERFACE)
target_compile_options(
  pie_binary
  INTERFACE
  -ffreestanding
  -fPIE
  -msingle-pic-base
  -mno-pic-data-is-text-relative
  -fomit-frame-pointer
)
# Some libraries are compiled with `-ffunction-sections` as they
# contain a lot of inline definitions that are needed for correctness,
# but are sparsely used.
# Make sure we only link what we use by garbage collecting sections.
# BUG: The resulting binaries do not actually have the the functions
# in question removed.
# https://stackoverflow.com/questions/56283941/why-gcc-gc-sections-and-ffunction-sections-is-not-working-cmake-build
# indicates that this is because -Wl,--gc-sections is incompatible
# with dynamic executables, even if only a selected set of symbols is
# exported.
target_link_options(
  pie_binary
  INTERFACE
  -fvisibility=hidden
  -Wl,--gc-sections
)
link_libraries(pie_binary)

add_subdirectory(kern)
add_subdirectory(root)
add_subdirectory(lib)

add_subdirectory(dist)
file(
  CREATE_LINK
  ${CMAKE_CURRENT_BINARY_DIR}/dist/bradypos.bin
  ${CMAKE_CURRENT_BINARY_DIR}/bradypos.bin
  SYMBOLIC
)
