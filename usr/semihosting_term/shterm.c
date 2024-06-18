#include <l4/ipc.h>
#include <l4/thread.h>
#include <l4/utcb.h>

enum
{
    SHTERM_STACK_SIZE = 512,
};

register unsigned char *got_location asm("r9");

__attribute__((naked)) void _start() { asm("bl main\n\t"); }

extern L4_utcb_t __utcb;

int main() {}
