#ifndef BRADYPOS_PLATFORM_H
#define BRADYPOS_PLATFORM_H

#include <kern/hardware.h>
#include <stdint.h>

extern const uintptr_t __vector[];
extern void _start();

inline void enable_interrupts() { __asm__ volatile("cpsie i"); }
inline void disable_interrupts() { __asm__ volatile("cpsid i"); }
inline void dsb() { asm volatile("dsb" ::: "memory"); }
inline void isb() { asm volatile("isb" ::: "memory"); }
#endif
