#include <kern/memory.h>
#include <kern/syscall.h>

void syscall_unmap() { unblock_caller(); }
