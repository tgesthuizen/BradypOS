cmake_minimum_required(VERSION 3.20)

project(
  semihosting_term
  VERSION 0.1.0
  LANGUAGES C
)

add_executable(
  semihosting_term
  shterm.c
  data.s
)
target_link_libraries(
  semihosting_term
  PUBLIC
  l4
  semihosting
)

target_link_options(
  semihosting_term
  PUBLIC
  -Wl,-E,-z,text,-z,common-page-size=512,-z,max-page-size=512
  -T ${BRADYPOS_USER_LINKER_SCRIPT}
  -Wl,--version-script,${BRADYPOS_USER_VERSION_SCRIPT}
  -pie
)
set_target_properties(
  semihosting_term
  PROPERTIES
  OUTPUT_NAME shterm
  LINK_DEPENDS "${BRADYPOS_USER_LINKER_SCRIPT};${BRADYPOS_USER_VERSION_SCRIPT}"
  INIT_TASK spawn
)
