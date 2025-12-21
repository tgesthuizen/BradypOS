#include "string.h"

size_t strlen(const char *str)
{
    size_t len = 0;
    while (*str++)
    {
        ++len;
    }
    return len;
}

int strcmp(const char *lhs, const char *rhs)
{
    while (*lhs && *rhs)
    {
        const int diff = *lhs - *rhs;
        if (diff)
            return diff;
        ++lhs;
        ++rhs;
    }
    return (lhs ? *lhs : 0) - (rhs ? *rhs : 0);
}

void *memset(void *dest, int ch, size_t size)
{
    char *dest_c = dest;
    while (size--)
    {
        *dest_c++ = ch;
    }
    return dest;
}

int memcmp(const void *lhs, const void *rhs, size_t count)
{
    const char *lhs_c = lhs;
    const char *rhs_c = rhs;
    while (count--)
    {
        int diff = *lhs_c++ - *rhs_c++;
        if (diff)
            return diff;
    }
    return 0;
}

void *memcpy(void *dest, const void *src, size_t size)
{
    return memmove(dest, src, size);
}

void *memmove(void *dest, const void *src, size_t size)
{
    char *dest_c = dest;
    const char *src_c = src;
    while (size--)
    {
        *dest_c++ = *src_c++;
    }
    return dest;
}
