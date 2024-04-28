cmake_minimum_required(VERSION 3.20)

# SSP support for the kernel:
# The library contains code that will be called when the stack canary
# is found damaged.
# And it defines the stack canary itself.
# In order to facilitate the instrumentation of the code, linking
# libraries will be compiled with `-fstack-protector`. However, this
# does not apply to ssp.c itself.

add_library(kern_ssp STATIC ssp.c)
target_compile_options(
  kern_ssp
  INTERFACE
  -fstack-protector
)
target_link_libraries(kern_ssp PUBLIC kernel_headers)
# Don't build this module when it is not required
set_target_properties(
  kern_ssp
  PROPERTIES
  EXCLUDE_FROM_ALL ON
)

option(
  BRADYPOS_KERNEL_USE_SSP
  "Whether the kernel should be build with stack smash protection"
  $<$<strequal:${CMAKE_BUILD_TYPE}:Debug>:ON:OFF>
)

if(BRADYPOS_KERNEL_USE_SSP)
  target_link_libraries(kern PRIVATE kern_ssp)
endif()
