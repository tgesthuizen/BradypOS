#include <l4/ipc.h>
#include <l4/schedule.h>
#include <l4/thread.h>
#include <l4/utcb.h>

enum
{
    SHTERM_STACK_SIZE = 512,
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

extern L4_utcb_t __utcb;

int main()
{
    while (1)
    {
        L4_yield();
    }
}
