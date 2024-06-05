#ifndef L4_TIME_H
#define L4_TIME_H

#include <stdint.h>

typedef union
{
    struct
    {
        unsigned m : 10;
        unsigned e : 5;
        unsigned point : 1;
    };
    uint16_t raw;
} L4_time_t;

#define L4_never ((L4_time_t){.raw = 0})
#define L4_zero_time ((L4_time_t){.point = 0, .e = 1, .m = 0})

#endif
