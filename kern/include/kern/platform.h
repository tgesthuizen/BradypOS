#ifndef BRADYPOS_PLATFORM_H
#define BRADYPOS_PLATFORM_H

#include "hardware.h"
#include <stdint.h>

extern const uintptr_t __vector[];
extern void _start();

static inline void enable_interrupts() { __asm__ volatile("cpsie i"); }
static inline void disable_interrupts() { __asm__ volatile("cpsid i"); }

#endif
