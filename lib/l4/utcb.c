#include <l4/utcb.h>

__attribute__((weak)) L4_utcb_t *L4_my_utcb()
{
    extern L4_utcb_t __utcb;
    return &__utcb;
}
