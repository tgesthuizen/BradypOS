cmake_minimum_required(VERSION 3.20)

project(
  shterm
  LANGUAGES C
  VERSION 0.1.0
)

add_executable(
  shterm
  shterm.c
  data.s
)
target_link_libraries(
  shterm
  PUBLIC
  l4
  semihosting
  libstring
)

target_link_options(
  shterm
  PUBLIC
  -Wl,-E,-z,text,-z,common-page-size=512,-z,max-page-size=512
  -T ${BRADYPOS_USER_LINKER_SCRIPT}
  -Wl,--version-script,${BRADYPOS_USER_VERSION_SCRIPT}
  -pie
)
set_target_properties(
  shterm
  PROPERTIES
  OUTPUT_NAME shterm
  LINK_DEPENDS "${BRADYPOS_USER_LINKER_SCRIPT};${BRADYPOS_USER_VERSION_SCRIPT}"
  INIT_TASK spawn
)
