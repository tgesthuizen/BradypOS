#include "kern/hardware.h"
#include "types.h"
#include <kern/platform.h>
#include <kern/thread.h>
#include <l4/thread.h>
#include <stddef.h>

enum
{
    THREAD_MAX_COUNT = 16,
    THREAD_IDX_INVALID,
};

static struct tcb_t tcb_store[THREAD_MAX_COUNT];
static unsigned tcb_store_allocated;
static unsigned thread_count;
static unsigned char thread_list[THREAD_MAX_COUNT];
static inline bool compare_thread_by_prio(unsigned char lhs, unsigned char rhs)
{
    return tcb_store[lhs].priority < tcb_store[rhs].priority;
}

#define HEAP_VALUE_TYPE unsigned char
#define HEAP_SIZE THREAD_MAX_COUNT
#define HEAP_COMPARE(l, r) compare_thread_by_prio((l), (r))
#define HEAP_PREFIX thread_schedule
#include <heap.inc>
#undef HEAP_PREFIX
#undef HEAP_COMPARE
#undef HEAP_SIZE
#undef HEAP_VALUE_TYPE
static unsigned current_thread_idx;

enum icsr_bits_t
{
    ICSR_VECT_ACTIVE = (1 << 9) - 1,
    ICSR_VECT_PENDING = ((1 << 9) - 1) << 12,
    ICSR_ISR_PENDING = 1 << 22,
    ICSR_ISR_PREEMPT = 1 << 23,
    ICSR_PENDSTCLR = 1 << 25,
    ICSR_PENDSTSET = 1 << 26,
    ICSR_PENDSVCLR = 1 << 27,
    ICSR_PENDSVSET = 1 << 28,
    ICSR_NMIPENDSET = 1 << 31,
};

void init_thread_system()
{
    tcb_store_allocated = 0;
    thread_count = 0;
    current_thread_idx = THREAD_IDX_INVALID;
}

static unsigned thread_map_find(L4_thread_t global_id)
{
    int high = thread_count;
    int low = 0;
    unsigned thread_no = L4_thread_no(global_id);
    while (low <= high)
    {
        if (high - low <= 1)
            return thread_list[low];
        const unsigned mid = low + (high - low) / 2;
        if (L4_thread_no(tcb_store[thread_list[mid]].global_id) >= thread_no)
            high = mid;
        else
            low = mid;
    }
    return THREAD_IDX_INVALID;
}

static void swap_unsigned_char(unsigned char *lhs, unsigned char *rhs)
{
    const unsigned tmp = *lhs;
    *lhs = *rhs;
    *rhs = tmp;
}

struct tcb_t *allocate_tcb()
{
    for (unsigned i = 0; i < THREAD_MAX_COUNT; ++i)
    {
        if (tcb_store_allocated & (1 << i))
        {
            tcb_store_allocated |= (1 << i);
            return tcb_store + i;
        }
    }
    return NULL;
}

struct tcb_t *insert_thread(struct utcb_t *utcb, L4_thread_t global_id)
{
    if (L4_version(global_id) == 0)
        return NULL;
    if (L4_thread_no(global_id) < 48)
        return NULL;
    if (thread_count == THREAD_MAX_COUNT)
        return NULL;
    unsigned idx = thread_map_find(global_id);
    if (idx == THREAD_IDX_INVALID)
        return NULL;

    struct tcb_t *tcb = allocate_tcb();
    tcb->global_id = global_id;
    tcb->local_id = 0;
    tcb->as = NULL;
    tcb->priority = 42; // TODO
    tcb->state = TS_INACTIVE;
    tcb->utcb = utcb;
    unsigned char tmp = tcb - tcb_store;
    for (unsigned i = idx; i < thread_count + 1; ++i)
    {
        swap_unsigned_char(&tmp, thread_list + i);
    }
    ++thread_count;
    return &tcb_store[thread_list[idx]];
}

void request_reschedule()
{
    *(volatile unsigned *)(PPB_BASE + ICSR_OFFSET) |= ICSR_PENDSVSET;
}

void schedule_next_thread()
{
    if (current_thread_idx != THREAD_IDX_INVALID &&
        tcb_store[current_thread_idx].state == TS_RUNNABLE)
        thread_schedule_insert(current_thread_idx);
    const unsigned idx = thread_schedule_state.data[0];
    thread_schedule_pop();
    current_thread_idx = idx;
}

static unsigned char reverse_lookup_thread(struct tcb_t *thread)
{
    const unsigned char target_idx = thread - tcb_store;
    for (unsigned char i = 0; i < thread_count; ++i)
    {
        if (thread_list[i] == target_idx)
            return i;
    }
    return THREAD_IDX_INVALID;
}

static unsigned find_idx_in_heap(unsigned idx)
{
    for (unsigned i = 0; i < thread_schedule_state.size; ++i)
    {
        if (thread_schedule_state.data[i] == idx)
            return i;
    }
    return thread_schedule_fail;
}

void set_thread_state(struct tcb_t *thread, enum thread_state_t state)
{
    const unsigned idx = thread - tcb_store;
    if (state == TS_RUNNABLE && thread->state != TS_RUNNABLE)
    {
        thread_schedule_insert(idx);
    }
    else if (state != TS_RUNNABLE && thread->state == TS_RUNNABLE)
    {
        thread_schedule_delete(find_idx_in_heap(idx));
    }
    thread->state = state;
}

struct tcb_t *thread_tcb(L4_thread_t thread)
{
    const int idx = thread_map_find(thread);
    if (idx == -1)
        return NULL;
    return &tcb_store[thread_list[idx]];
}

struct tcb_t *get_current_thread() { return &tcb_store[current_thread_idx]; }

static __attribute__((used)) struct thread_context_t *get_current_ctx()
{
    return &get_current_thread()->ctx;
}

__attribute__((naked)) void isr_pendsv()
{
    asm volatile(
        // Store context on stack
        "movs  r1, sp\n\t" // Save original sp
        "push  {r4,r7}\n\t"
        "movs  r4, r8\n\t"
        "movs  r5, r9\n\t"
        "movs  r6, r10\n\t"
        "movs  r7, r11\n\t"
        "push  {r4,r7}\n\t"
        "mrs   r0, psp\n\t"
        "push  {r0, lr}\n\t"
        // Restore GOT location
        "ldr   r0, =#0x20040000\n\t"
        "ldr   r0, [r0]\n\t"
        "movs  r9, r0\n\t"
        "movs  r4, r1\n\t" // Save original stack pointer so it won't get
                           // clobbered
        // Copy saved context on stack into tcb
        "bl    get_current_ctx\n\t"
        "cmp   r0, #0\n\t"
        "beq   .Lschedule_next\n\t"
        "movs  r1, r4\n\t"
        "movs  r2, #40\n\t"
        "bl    memcpy\n"
        // Schedule next thread
        ".Lschedule_next:\n\t"
        "movs  sp, r4\n\t" // Pop context of the stack
        "bl    schedule_next_thread\n\t"
        // Restore new current context
        "bl    get_current_ctx\n\t"
        "ldmia r0!, {r1, r2}\n\t"
        "movs  sp, r1\n\t"
        "msr   psp, r1\n\t"
        "movs  lr, r2\n\t"
        "ldmia r0!, {r4-r7}\n\t"
        "movs  r8, r4\n\t"
        "movs  r9, r5\n\t"
        "movs  r10, r6\n\t"
        "movs  r11, r7\n\t"
        "ldmia r0!, {r4-r7}\n\t"
        "bx lr\n\t");
    // TODO: Pass in functions as input operands. GCC is really doing anything
    // in its power not to accept it.
}

void start_scheduling() { request_reschedule(); }
