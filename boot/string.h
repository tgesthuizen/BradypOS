#ifndef BRADYPOS_BOOT_STRING_H
#define BRADYPOS_BOOT_STRING_H

#include <stddef.h>

inline void *memset(void *ptr, int value, size_t num)
{
    unsigned char *cptr = (unsigned char *)ptr;
    while (num--)
    {
        *cptr++ = value;
    }
    return ptr;
}

inline void *memcpy(void *dest, const void *src, size_t size)
{
    unsigned char *cdest = (unsigned char *)dest;
    const unsigned char *csrc = (const unsigned char *)src;
    while (size--)
    {
        *cdest++ = *csrc++;
    }
    return dest;
}

#endif
