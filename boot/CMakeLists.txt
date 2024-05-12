cmake_minimum_required(VERSION 3.20)

project(boot
  LANGUAGES C ASM
  VERSION 0.0.1
)

set(LIBELF_TARGET_POSTFIX -static)
add_subdirectory(../lib/elf elf)
set(LIBROMFS_TARGET_POSTFIX -static)
add_subdirectory(../lib/romfs romfs)
set(LIBSTRING_TARGET_POSTFIX -static)
add_subdirectory(../lib/string string)

add_executable(boot
  bs2_default_padded_checksummed.S
  boot.c
  bootinfo.c
)
target_link_libraries(
  boot
  PUBLIC
  l4_common
  libelf-static
  libromfs-static
  libstring-static
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
  OUTPUT_NAME boot.elf
)
