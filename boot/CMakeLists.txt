cmake_minimum_required(VERSION 3.20)

project(boot
  LANGUAGES C ASM
  VERSION 0.0.1
)

add_executable(boot
  bs2_default_padded_checksummed.S
  boot.c
)
target_link_libraries(
  boot
  PUBLIC
  libromfs
)

target_compile_options(boot
  PUBLIC
  -ffreestanding
)

set(BOOT_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/bootloader.ld)
target_link_options(
  boot
  PUBLIC
  -T ${BOOT_LINKER_SCRIPT}
)
set_target_properties(
  boot
  PROPERTIES
  LINK_DEPENDS ${BOOT_LINKER_SCRIPT}
)
