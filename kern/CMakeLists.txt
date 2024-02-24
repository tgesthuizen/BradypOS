cmake_minimum_required(VERSION 3.20)

add_library(kern_common INTERFACE)
target_include_directories(kern_common INTERFACE include)

add_executable(
  kern

  debug.c
  main.c
  semihosting.c
  string.c
  vector.c
)

target_include_directories(
  kern
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIRECTORY}
)
target_link_libraries(kern PUBLIC kern_common)

add_subdirectory(platform/${BRADYPOS_TARGET_PLATFORM})
add_subdirectory(ssp)
