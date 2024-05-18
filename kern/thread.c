#include <kern/debug.h>
#include <kern/hardware.h>
#include <kern/kalarm.h>
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

static unsigned thread_list_find(L4_thread_id global_id)
{
    unsigned high = thread_count;
    unsigned low = 0;
    unsigned thread_no = L4_thread_no(global_id);
    while (low <= high)
    {
        if (high - low <= 1)
            return low;
        const unsigned mid = low + (high - low) / 2;
        if (L4_thread_no(tcb_store[thread_list[mid]].global_id) >= thread_no)
            high = mid;
        else
            low = mid;
    }
    return THREAD_IDX_INVALID;
}

static unsigned thread_map_find(L4_thread_id global_id)
{
    unsigned idx = thread_list_find(global_id);
    if (idx == THREAD_IDX_INVALID)
        return THREAD_IDX_INVALID;
    return thread_list[idx];
}

struct tcb_t *find_thread_by_global_id(L4_thread_id global_id)
{
    const unsigned idx = thread_map_find(global_id);
    if (idx == THREAD_IDX_INVALID)
        return NULL;
    return &tcb_store[idx];
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
            continue;
        tcb_store_allocated |= (1 << i);
        return tcb_store + i;
    }
    return NULL;
}

struct tcb_t *insert_thread(L4_utcb_t *utcb, L4_thread_id global_id)
{
    if (L4_version(global_id) == 0)
        return NULL;
    if (thread_count == THREAD_MAX_COUNT)
        return NULL;
    unsigned idx = thread_list_find(global_id);
    if (idx == THREAD_IDX_INVALID)
        return NULL;

    struct tcb_t *tcb = allocate_tcb();
    if (!tcb)
        return NULL;
    tcb->global_id = global_id;
    tcb->local_id = (unsigned)utcb;
    tcb->pager = get_kernel_tcb()->global_id;
    tcb->scheduler = get_kernel_tcb()->global_id;
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
    if (utcb)
        write_utcb(tcb);
    return &tcb_store[thread_list[idx]];
}

static struct tcb_t *schedule_target;

void request_reschedule(struct tcb_t *target)
{
    schedule_target = target;
    *(volatile unsigned *)(PPB_BASE + ICSR_OFFSET) = ICSR_PENDSVSET;
}

static unsigned find_thread_idx_to_schedule()
{
    if (thread_schedule_state.size == 0)
        panic("No task can be scheduled!\n");

    const unsigned idx = thread_schedule_state.data[0];
    thread_schedule_pop();
    return idx;
}

static unsigned find_thread_idx_to_schedule_from_target()
{
    unsigned idx = 0;
    const unsigned target_thread_map_idx = schedule_target - tcb_store;
    for (idx = 0; idx < thread_schedule_state.size; ++idx)
    {
        if (thread_schedule_state.data[idx] == target_thread_map_idx)
        {
            thread_schedule_delete(idx);
            if (schedule_target->state != TS_RUNNABLE)
            {
                panic("Thread %u is not active, but shall be scheduled",
                      schedule_target->global_id);
            }
            return target_thread_map_idx;
        }
    }
    return find_thread_idx_to_schedule();
}

void schedule_next_thread()
{
    if (current_thread_idx != THREAD_IDX_INVALID &&
        tcb_store[current_thread_idx].state == TS_RUNNABLE)
        thread_schedule_insert(current_thread_idx);

    const unsigned idx = schedule_target != NULL
                             ? find_thread_idx_to_schedule_from_target()
                             : find_thread_idx_to_schedule();
    // Disable interrupts so that systick cannot read the kalarm heap with a
    // race condition.
    disable_interrupts();
    // TODO: Actual quota management
    register_kalarm_event(get_current_time() + 10, request_reschedule);
    enable_interrupts();
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
        const unsigned heap_idx = find_idx_in_heap(idx);
        if (heap_idx != thread_schedule_fail)
            thread_schedule_delete(heap_idx);
    }
    thread->state = state;
}

struct tcb_t *thread_tcb(L4_thread_id thread)
{
    const int idx = thread_map_find(thread);
    if (idx == -1)
        return NULL;
    return &tcb_store[thread_list[idx]];
}

struct tcb_t *get_current_thread()
{
    if (current_thread_idx == THREAD_IDX_INVALID)
        return NULL;
    return &tcb_store[current_thread_idx];
}

static __attribute__((used)) struct thread_context_t *get_current_ctx()
{
    struct tcb_t *current_thread = get_current_thread();
    if (!current_thread)
        return NULL;
    return &current_thread->ctx;
}

static struct thread_context_t *__attribute__((used))
switch_context(struct thread_context_t *current_context)
{
    struct thread_context_t *tcb_ctx = get_current_ctx();
    if (tcb_ctx != NULL)
        *tcb_ctx = *current_context;
    schedule_next_thread();
    return get_current_ctx();
}

__attribute__((naked)) void isr_pendsv()
{
    asm volatile(
        // Store context on stack
        "movs  r1, sp\n\t" // Save original sp
        "push  {r4-r7}\n\t"
        "movs  r4, r8\n\t"
        "movs  r5, r9\n\t"
        "movs  r6, r10\n\t"
        "movs  r7, r11\n\t"
        "push  {r4-r7}\n\t"
        "mrs   r0, psp\n\t"
        "push  {r0, lr}\n\t"
        // Restore GOT location
        "ldr   r0, =#0x20040000\n\t"
        "ldr   r0, [r0]\n\t"
        "movs  r9, r0\n\t"
        "movs  r4, r1\n\t" // Save original stack pointer so it won't get
                           // clobbered
        "movs  r0, sp\n\t"
        "bl    switch_context\n\t"
        "movs  sp, r4\n\t"
        "ldmia r0!, {r1, r2}\n\t"
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

void start_scheduling()
{
    request_reschedule(NULL);
    while (1)
        asm volatile("wfi");
}
