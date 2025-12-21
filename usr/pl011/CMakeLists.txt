cmake_minimum_required(VERSION 3.20)

project(pl011
  LANGUAGES C ASM
  VERSION 0.1.1
)

add_executable(
  pl011

  data.s
  pl011.c
)

target_link_libraries(
  pl011
  PUBLIC
  l4
  libroot
  librp2040_utils
  libstring
)

target_link_options(
  pl011
  PUBLIC
  -Wl,-E,-z,text,-z,common-page-size=512,-z,max-page-size=512
  -T ${BRADYPOS_USER_LINKER_SCRIPT}
  -Wl,--version-script,${BRADYPOS_USER_VERSION_SCRIPT}
  -pie
)
set_target_properties(
  pl011
  PROPERTIES
  OUTPUT_NAME pl011
  LINK_DEPENDS "${BRADYPOS_USER_LINKER_SCRIPT};${BRADYPOS_USER_VERSION_SCRIPT}"
  INIT_TASK spawn
)
