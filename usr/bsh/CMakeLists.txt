cmake_minimum_required(VERSION 3.20)

project(
  bsh
  LANGUAGES C ASM
  VERSION 0.1.0
)

add_executable(
  bsh

  bsh.c
  data.s
)
target_link_libraries(
  bsh
  PUBLIC
  l4
  libstring
  libvfs
)
target_link_options(
  bsh
  PUBLIC
  -Wl,-E,-z,text,-z,common-page-size=512,-z,max-page-size=512
  -T ${BRADYPOS_USER_LINKER_SCRIPT}
  -Wl,--version-script,${BRADYPOS_USER_VERSION_SCRIPT}
  -pie
)
set_target_properties(
  bsh
  PROPERTIES
  OUTPUT_NAME bsh
  LINK_DEPENDS "${BRADYPOS_USER_LINKER_SCRIPT};${BRADYPOS_USER_VERSION_SCRIPT}"
  INIT_TASK spawn
)
