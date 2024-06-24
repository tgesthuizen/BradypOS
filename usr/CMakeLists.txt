set(BRADYPOS_USER_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/user.ld)
set(BRADYPOS_USER_VERSION_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/version.ld)

add_subdirectory(bsh)
add_subdirectory(root)
add_subdirectory(shterm)
