# TODO: Only built when actually on Linux
add_executable(elfdemo-linux linux.c)
target_link_libraries(elfdemo-linux PUBLIC libelf)
