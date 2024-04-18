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
               "bl [main]\n\t"
               "b ."
               :
               : [main] "m"(main)
               : "r0", "lr");
}

extern void *romfs_start;

static int main() {}
