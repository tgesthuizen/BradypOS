set(BRADYPOS_SBIN_COMPONENTS kern root)
set(BRADYPOS_BIN_COMPONENTS shterm)
set(BRADYPOS_LIB_COMPONENTS libelf libromfs)

find_program(GENROMFS_EXECUTABLE genromfs REQUIRED)

add_custom_target(
  clean-romfs
  COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_CURRENT_BINARY_DIR}/rootfs
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/rootfs
)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/rootfs)

set(romfs_dependencies "")

function(place_stripped_bin_in_rootfs dir target)
  get_target_property(OUTPUT_NAME ${target} OUTPUT_NAME)
  set(TARGET_FILE ${CMAKE_CURRENT_BINARY_DIR}/rootfs/${dir}/${OUTPUT_NAME})
  set(SRC_FILE $<TARGET_FILE:${target}>)
  add_custom_command(
    OUTPUT ${TARGET_FILE}
    COMMAND ${CMAKE_COMMAND} -E copy ${SRC_FILE} ${TARGET_FILE}
    COMMAND ${CMAKE_OBJCOPY} -S ${TARGET_FILE}
    DEPENDS ${SRC_FILE}
    COMMENT "Installing ${target} in rootfs"
  )
  list(APPEND romfs_dependencies ${TARGET_FILE})
  set(romfs_dependencies ${romfs_dependencies} PARENT_SCOPE)
endfunction()

configure_file(version.in version)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/rootfs/etc/version
  COMMAND ${CMAKE_COMMAND} -E copy
          ${CMAKE_CURRENT_BINARY_DIR}/version
	  ${CMAKE_CURRENT_BINARY_DIR}/rootfs/etc/version
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/version
  COMMENT "Installing /etc/version in rootfs"
)
list(APPEND romfs_dependencies
  ${CMAKE_CURRENT_BINARY_DIR}/rootfs/etc/version)

foreach(bin ${BRADYPOS_SBIN_COMPONENTS})
  place_stripped_bin_in_rootfs("sbin" ${bin})
endforeach()
set(inittab "")
foreach(bin ${BRADYPOS_BIN_COMPONENTS})
  place_stripped_bin_in_rootfs("bin" "${bin}")
  get_target_property(inittype ${bin} INIT_TASK)
  if(NOT (inittype STREQUAL "inittype-NOTFOUND"))
    get_target_property(bin_name ${bin} OUTPUT_NAME)
    set(inittab "${inittab}${inittype}:/bin/${bin_name}\n")
  endif()
endforeach()
set(INITTAB_FILE "${CMAKE_CURRENT_BINARY_DIR}/inittab")
file(GENERATE OUTPUT "${INITTAB_FILE}" CONTENT "${inittab}")
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/rootfs/etc/inittab
  COMMAND ${CMAKE_COMMAND} -E copy
  	  ${INITTAB_FILE}
	  ${CMAKE_CURRENT_BINARY_DIR}/rootfs/etc/inittab
  DEPENDS ${INITTAB_FILE}
  COMMENT "Installing /etc/inittab in rootfs"
)
list(APPEND romfs_dependencies
  ${CMAKE_CURRENT_BINARY_DIR}/rootfs/etc/inittab)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/root.romfs
  COMMAND ${GENROMFS_EXECUTABLE}
          -V bradypos
	  -d ${CMAKE_CURRENT_BINARY_DIR}/rootfs
	  -f ${CMAKE_CURRENT_BINARY_DIR}/root.romfs
	  -A '512,/sbin/*' -A '512,/bin/*'
  DEPENDS ${romfs_dependencies}
  COMMENT "Generating root.romfs"
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/boot.bin
  COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:boot> ${CMAKE_CURRENT_BINARY_DIR}/boot.bin
  DEPENDS boot
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bradypos.bin
  COMMAND OBJDUMP=${CMAKE_OBJDUMP}
          ${CMAKE_CURRENT_SOURCE_DIR}/create_image.sh
          $<TARGET_FILE:boot>
	  ${CMAKE_CURRENT_BINARY_DIR}/boot.bin
	  ${CMAKE_CURRENT_BINARY_DIR}/root.romfs
	  ${CMAKE_CURRENT_BINARY_DIR}/bradypos.bin
  DEPENDS boot
	  ${CMAKE_CURRENT_SOURCE_DIR}/create_image.sh
          ${CMAKE_CURRENT_BINARY_DIR}/boot.bin
	  ${CMAKE_CURRENT_BINARY_DIR}/root.romfs
  COMMENT "Constructing flash binary"
)

add_custom_target(
  image
  ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/bradypos.bin
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/bradypos.bin
  DESTINATION ${CMAKE_INSTALL_DATADIR}
)
