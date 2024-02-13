#include <stdint.h>

extern uint8_t __stack[];
extern void _start();

static __attribute__((naked)) void isr_invalid() {
  __asm__ volatile("bkpt #0\n");
}
void __attribute__((weak, alias("isr_invalid"))) isr_hardfault();
void __attribute__((weak, alias("isr_invalid"))) isr_systick();
void __attribute__((weak, alias("isr_invalid"))) isr_nmi();
void __attribute__((weak, alias("isr_invalid"))) isr_svcall();
void __attribute__((weak, alias("isr_invalid"))) isr_pendsv();

static __attribute__((naked)) void unhandled_irq() {
  __asm__ volatile("mrs r0, ipsr\n\t"
                   "sub r0, #16\n\t"
                   ".global unhandled_user_irq_num_in_r0\n"
                   "unhandled_user_irq_num_in_r0:\n\t"
                   "bkpt #0\n\t");
}
void __attribute__((weak, alias("unhandled_irq"))) isr_irq0();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq1();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq2();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq3();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq4();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq5();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq6();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq7();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq8();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq9();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq10();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq11();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq12();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq13();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq14();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq15();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq16();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq17();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq18();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq19();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq20();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq21();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq22();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq23();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq24();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq25();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq26();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq27();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq28();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq29();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq30();
void __attribute__((weak, alias("unhandled_irq"))) isr_irq31();

uintptr_t __vector[] = {
    (uintptr_t)__stack,       (uintptr_t)_start,      (uintptr_t)isr_nmi,
    (uintptr_t)isr_hardfault, (uintptr_t)isr_invalid, (uintptr_t)isr_invalid,
    (uintptr_t)isr_invalid,   (uintptr_t)isr_invalid, (uintptr_t)isr_invalid,
    (uintptr_t)isr_invalid,   (uintptr_t)isr_invalid, (uintptr_t)isr_svcall,
    (uintptr_t)isr_invalid,   (uintptr_t)isr_invalid, (uintptr_t)isr_pendsv,
    (uintptr_t)isr_systick,   (uintptr_t)isr_irq0,    (uintptr_t)isr_irq1,
    (uintptr_t)isr_irq2,      (uintptr_t)isr_irq3,    (uintptr_t)isr_irq4,
    (uintptr_t)isr_irq5,      (uintptr_t)isr_irq6,    (uintptr_t)isr_irq7,
    (uintptr_t)isr_irq8,      (uintptr_t)isr_irq9,    (uintptr_t)isr_irq10,
    (uintptr_t)isr_irq11,     (uintptr_t)isr_irq12,   (uintptr_t)isr_irq13,
    (uintptr_t)isr_irq14,     (uintptr_t)isr_irq15,   (uintptr_t)isr_irq16,
    (uintptr_t)isr_irq17,     (uintptr_t)isr_irq18,   (uintptr_t)isr_irq19,
    (uintptr_t)isr_irq20,     (uintptr_t)isr_irq21,   (uintptr_t)isr_irq22,
    (uintptr_t)isr_irq23,     (uintptr_t)isr_irq24,   (uintptr_t)isr_irq25,
    (uintptr_t)isr_irq26,     (uintptr_t)isr_irq27,   (uintptr_t)isr_irq28,
    (uintptr_t)isr_irq29,     (uintptr_t)isr_irq30,   (uintptr_t)isr_irq31};
