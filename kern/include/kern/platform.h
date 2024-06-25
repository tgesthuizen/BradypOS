#ifndef BRADYPOS_PLATFORM_H
#define BRADYPOS_PLATFORM_H

#include <kern/hardware.h>
#include <stdint.h>

extern const uintptr_t __vector[];
extern void _start();

inline void enable_interrupts() { __asm__ volatile("cpsie i" ::: "memory"); }
inline void disable_interrupts() { __asm__ volatile("cpsid i" ::: "memory"); }
inline void dsb() { asm volatile("dsb" ::: "memory"); }
inline void isb() { asm volatile("isb" ::: "memory"); }
inline unsigned *get_msp()
{
    unsigned *res;
    asm volatile("mrs %0, msp" : "=l"(res)::"memory");
    return res;
}
inline unsigned *get_psp()
{
    unsigned *res;
    asm volatile("mrs %0, psp" : "=l"(res)::"memory");
    return res;
}
#endif
