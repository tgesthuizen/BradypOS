#include <l4/schedule.h>
#include <l4/thread.h>

enum
{
    IPC_BUFFER_SIZE = 128,
};

register unsigned char *got_location asm("r9");

__attribute__((naked)) void _start()
{
    asm("pop {r0, r1}\n\t"
        "movs r9, r1\n\t"
        "movs r2, #0\n\t"
        "movs lr, r2\n\t"
        "bl main\n\t");
}

static unsigned char ipc_buffer[IPC_BUFFER_SIZE];

int main()
{
    while (1)
    {
        L4_yield();
    }
}
