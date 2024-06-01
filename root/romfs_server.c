#include "l4/thread.h"
#include "variables.h"
#include <romfs.h>

__attribute__((noreturn)) void romfs_start()
{
    while (1)
    {
    }
    // In case we reach the end of this program, delete us
    L4_thread_control(romfs_thread_id, L4_NILTHREAD, -1, -1, (void *)-1);
}
