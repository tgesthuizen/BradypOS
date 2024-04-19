#include "romfs.h"
#include <stddef.h>
#include <stdint.h>

extern unsigned char __stack[];
static int main();
void _start();
static __attribute__((naked)) void isr_invalid() {
  __asm__ volatile("bkpt #0\n");
}
static __attribute__((naked)) void unhandled_irq() {
  __asm__ volatile("mrs r0, ipsr\n\t"
                   "sub r0, #16\n\t"
                   ".global unhandled_user_irq_num_in_r0\n"
                   "unhandled_user_irq_num_in_r0:\n\t"
                   "bkpt #0\n\t");
}

void *memset(void *ptr, int value, size_t num) {
  unsigned char *cptr = (unsigned char *)ptr;
  while (num--) {
    *cptr++ = value;
  }
  return ptr;
}

void *memcpy(void *dest, const void *src, size_t size) {
  unsigned char *cdest = (unsigned char *)dest;
  const unsigned char *csrc = (const unsigned char *)src;
  while (size--) {
    *cdest++ = *csrc++;
  }
  return dest;
}

__attribute__((section(".vector"))) const uintptr_t __vector[] = {
    (uintptr_t)__stack,       (uintptr_t)_start,
    (uintptr_t)isr_invalid,   (uintptr_t)isr_invalid,
    (uintptr_t)isr_invalid,   (uintptr_t)isr_invalid,
    (uintptr_t)isr_invalid,   (uintptr_t)isr_invalid,
    (uintptr_t)isr_invalid,   (uintptr_t)isr_invalid,
    (uintptr_t)isr_invalid,   (uintptr_t)isr_invalid,
    (uintptr_t)isr_invalid,   (uintptr_t)isr_invalid,
    (uintptr_t)isr_invalid,   (uintptr_t)isr_invalid,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq,
    (uintptr_t)unhandled_irq, (uintptr_t)unhandled_irq};

__attribute__((naked)) void _start() {
  asm volatile("movs r0, #0\n\t"
               "movs lr, r0\n\t"
               "ldr  r0, =__bss_start\n\t"
               "movs r1, #0\n\t"
               "ldr  r2, =__bss_len\n\t"
               "bl   memset\n\t"
               "ldr  r0, =__data_start\n\t"
               "ldr  r1, =__data_lma\n\t"
               "ldr  r2, =__data_len\n\t"
               "bl   memcpy\n\t"
               "bl   main\n\t"
               "b    ." ::
                   : "r0", "r1", "r2", "lr");
}

extern void *romfs_start;

static int main() {}
