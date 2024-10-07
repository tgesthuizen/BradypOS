#ifndef BRADYPOS_KERN_STRING_H
#define BRADYPOS_KERN_STRING_H

#include <stddef.h>

size_t strlen(const char *str);
int strcmp(const char *lhs, const char *rhs);
void *memset(void *dest, int ch, size_t size);
int memcmp(const void *lhs, const void *rhs, size_t count);
void *memcpy(void *dest, const void *src, size_t size);
void *memmove(void *dest, const void *src, size_t size);

#endif
