#include <kern/debug.h>
#include <kern/memory.h>
#include <kern/platform.h>
#include <kern/thread.h>
#include <l4/errors.h>
#include <l4/kip.h>
#include <l4/space.h>
#include <l4/thread.h>
#include <l4/utcb.h>
#include <stddef.h>

extern L4_kip_t the_kip;
L4_fpage_t kip_fpage;

enum
{
    MAX_ADDRESS_SPACES = 16
};

static struct as_t as_pool[MAX_ADDRESS_SPACES];
static unsigned as_allocated;

void init_memory()
{
    kip_fpage =
        L4_fpage_add_rights(L4_fpage_log2((unsigned)&the_kip, 10), L4_readable);
    as_allocated = 0;
}

static unsigned alloc_as()
{
    unsigned i;
    for (i = 0; i < MAX_ADDRESS_SPACES; ++i)
    {
        if (!(as_allocated & (1 << i)))
        {
            as_allocated |= (1 << i);
            break;
        }
    }
    return i;
}

struct as_t *create_as(struct tcb_t *owner)
{
    unsigned as_idx = alloc_as();
    if (as_idx == MAX_ADDRESS_SPACES)
    {
        panic("Could not allocate new address space");
    }
    struct as_t *result = &as_pool[as_idx];
    result->first_thread = owner;
    result->utcb_page = L4_nilpage;
    result->ref_count = 1;
    return result;
}

static __attribute__((used)) void __isr_hardfault(unsigned lr)
{
    dbg_log(DBG_INTERRUPT, "Executing HardFault\n");

    const bool fault_in_thread_mode = (lr & (1 << 3)) != 0;
    const bool uses_psp = (lr & (1 << 2)) != 0;

    if (!fault_in_thread_mode)
    {
        goto critical_fault;
    }
    struct tcb_t *const current_thread = get_current_thread();
    if (L4_thread_no(current_thread->global_id) < L4_USER_THREAD_START)
    {
        goto critical_fault;
    }

    dbg_log(DBG_SCHEDULE, "Hardfault in user thread %#08x, killing it\n",
            current_thread->global_id);
    if (L4_thread_no(current_thread->global_id) == L4_USER_THREAD_START)
    {
        panic("The root thread died!\n");
    }
    delete_thread(current_thread);
    request_reschedule(NULL);
    return;

critical_fault:
    unsigned *const faulted_sp = uses_psp ? get_psp() : get_msp();
    dbg_printf(
        "Context stored to %s\n"
        "mode:\t%s\n"
        "SP:\t0x%08x\n"
        "PSR:\t0x%08x\n"
        "r0:\t0x%08x\n"
        "r1:\t0x%08x\n"
        "r2:\t0x%08x\n"
        "r3:\t0x%08x\n"
        "r12:\t0x%08x\n"
        "lr:\t0x%08x\n"
        "pc:\t0x%08x\n",
        uses_psp ? "PSP" : "MSP", fault_in_thread_mode ? "thread" : "handler",
        (unsigned)faulted_sp, faulted_sp[THREAD_CTX_STACK_PSR],
        faulted_sp[THREAD_CTX_STACK_R0], faulted_sp[THREAD_CTX_STACK_R1],
        faulted_sp[THREAD_CTX_STACK_R2], faulted_sp[THREAD_CTX_STACK_R3],
        faulted_sp[THREAD_CTX_STACK_R12], faulted_sp[THREAD_CTX_STACK_LR],
        faulted_sp[THREAD_CTX_STACK_PC]);

    panic("!!! PANIC: Hardfault triggered\n");
}

__attribute__((naked)) void isr_hardfault()
{
    asm("movs r0, r9\n\t"
        "movs r1, lr\n\t"
        "push {r0, lr}\n\t"
#ifndef NDEBUG
        ".cfi_def_cfa_offset 8\n\t"
        ".cfi_offset 9, -8\n\t"
        ".cfi_offset 14, -4\n\t"
#endif
        "ldr r0, =0x20040000\n\t"
        "ldr r0, [r0]\n\t"
        "movs r9, r0\n\t"
        "movs r0, r1\n\t"
        "bl __isr_hardfault\n\t"
        "pop {r0}\n\t"
        "movs r9, r0\n\t"
        "pop {pc}\n\t");
}
