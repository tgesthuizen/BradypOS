#ifndef BRADYPOS_KERN_NVIC_H
#define BRADYPOS_KERN_NVIC_H

#include <stdbool.h>

enum exception_nums
{
    EXC_RESET = 1,
    EXC_NMI = 2,
    EXC_HARD_FAULT = 3,
    EXC_SVCALL = 11,
    EXC_PENDSV = 14,
    EXC_SYSTICK = 15,
    EXC_EXTERNAL_BASE = 16,
};

enum exception_return_codes
{
    EXCEPTION_RETURN_TO_HANDLER_ON_MAIN_STACK = 0xFFFFFFF1,
    EXCEPTION_RETURN_TO_THREAD_ON_MAIN_STACK = 0xFFFFFFF9,
    EXCEPTION_RETURN_TO_THREAD_ON_PROCESS_STACK = 0xFFFFFFFD,
};

static inline unsigned irq_to_exception_number(unsigned irq_num)
{
    return EXC_EXTERNAL_BASE + irq_num;
}

void interrupts_init();

bool nvic_is_enabled(unsigned interrupt);
void nvic_set_enabled(unsigned interrupt, bool enabled);
bool nvic_is_pending(unsigned interrupt);
void nvic_set_pending(unsigned interrupt, bool pends);

// TODO: Get GCC to accept referencing kern_stack_base here
#ifndef NDEBUG
#define DECLARE_ISR(isr, func)                                                 \
    __attribute__((naked)) void isr()                                          \
    {                                                                          \
        asm("movs r0, r9\n\t"                                                  \
            "push {r0, lr}\n\t"                                                \
            ".cfi_def_cfa_offset 8\n\t"                                        \
            ".cfi_offset 9, -8\n\t"                                            \
            ".cfi_offset 14, -4\n\t"                                           \
            "ldr r0, =0x20040000\n\t"                                          \
            "ldr r0, [r0]\n\t"                                                 \
            "mov r9, r0\n\t"                                                   \
            "bl " #func "\n\t"                                                 \
            "pop {r0}\n\t"                                                     \
            "movs r9, r0\n\t"                                                  \
            "pop {pc}\n\t");                                                   \
    }
#else
#define DECLARE_ISR(isr, func)                                                 \
    __attribute__((naked)) void isr()                                          \
    {                                                                          \
        asm("movs r0, r9\n\t"                                                  \
            "push {r0, lr}\n\t"                                                \
            "ldr r0, =0x20040000\n\t"                                          \
            "ldr r0, [r0]\n\t"                                                 \
            "mov r9, r0\n\t"                                                   \
            "bl " #func "\n\t"                                                 \
            "pop {r0}\n\t"                                                     \
            "movs r9, r0\n\t"                                                  \
            "pop {pc}\n\t");                                                   \
    }

#endif
#endif
