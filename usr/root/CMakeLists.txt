cmake_minimum_required(VERSION 3.20)

project(
  root
  LANGUAGES C
  VERSION 0.1.0
)

add_library(
  libroot
  INTERFACE
)
target_include_directories(
  libroot
  INTERFACE
  include
)

add_executable(
  root
  clocks.c
  config.c
  data.s
  exec.c
  memory.c
  resets.c
  romfs_server.c
  root.c
  service.c
)
target_compile_options(
  root
  PUBLIC
  -fvisibility=hidden
)
target_link_libraries(
  root
  PUBLIC
  l4
  libbuddy
  libelf
  libromfs
  libroot
  libstring
  libvfs
)
target_link_options(
  root
  PUBLIC
  -Wl,-E,-z,text,-z,common-page-size=512,-z,max-page-size=512
  -T ${BRADYPOS_USER_LINKER_SCRIPT}
  -Wl,--version-script,${BRADYPOS_USER_VERSION_SCRIPT}
  -pie
)
set_target_properties(
  root
  PROPERTIES
  OUTPUT_NAME root
  LINK_DEPENDS "${BRADYPOS_USER_LINKER_SCRIPT};${BRADYPOS_USER_VERSION_SCRIPT}"
)
