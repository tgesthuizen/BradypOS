message(INFO " Compiling for cortex-m0plus")

target_compile_options(
  platform
  INTERFACE
  -mcpu=cortex-m0plus
  -mthumb
)
target_link_options(
  platform
  INTERFACE
  -mcpu=cortex-m0plus
  -mthumb
)
