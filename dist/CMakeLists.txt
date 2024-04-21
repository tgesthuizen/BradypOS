set(BRADYPOS_SBIN_COMPONENTS kern root)
set(BRADYPOS_LIB_COMPONENTS libelf libromfs)

find_program(GENROMFS_EXECUTABLE genromfs REQUIRED)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/rootfs)

set(romfs_dependencies "")

macro(place_stripped_sbin_in_rootfs target)
  get_target_property(OUTPUT_NAME ${target} OUTPUT_NAME)
  set(TARGET_FILE ${CMAKE_CURRENT_BINARY_DIR}/rootfs/sbin/${OUTPUT_NAME})
  set(SRC_FILE $<TARGET_FILE:${target}>)
  add_custom_command(
    OUTPUT ${TARGET_FILE}
    COMMAND ${CMAKE_COMMAND} -E copy ${SRC_FILE} ${TARGET_FILE}
    COMMAND ${CMAKE_OBJCOPY} -S ${TARGET_FILE}
    DEPENDS ${SRC_FILE}
    COMMENT "Installing ${target} in rootfs"
  )
  list(APPEND romfs_dependencies ${TARGET_FILE})
endmacro()

place_stripped_sbin_in_rootfs(kern)
place_stripped_sbin_in_rootfs(root)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/root.romfs
  COMMAND ${GENROMFS_EXECUTABLE}
          -V bradypos
	  -d ${CMAKE_CURRENT_BINARY_DIR}/rootfs
	  -f ${CMAKE_CURRENT_BINARY_DIR}/root.romfs
	  -A 512,/sbin/*
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
  COMMAND cat ${CMAKE_CURRENT_BINARY_DIR}/boot.bin
              ${CMAKE_CURRENT_BINARY_DIR}/root.romfs
	      >${CMAKE_CURRENT_BINARY_DIR}/bradypos.bin
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/boot.bin ${CMAKE_CURRENT_BINARY_DIR}/root.romfs
  COMMENT "Constructing flash binary"
)

add_custom_target(
  image
  ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/bradypos.bin
)
