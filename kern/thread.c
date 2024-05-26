#include <kern/debug.h>
#include <kern/hardware.h>
#include <kern/kalarm.h>
#include <kern/memory.h>
#include <kern/platform.h>
#include <kern/systhread.h>
#include <kern/thread.h>
#include <l4/errors.h>
#include <l4/space.h>
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
    do
    {
        const unsigned mid = (low + high) / 2;
        const unsigned current_no =
            L4_thread_no(tcb_store[thread_list[mid]].global_id);
        if (current_no == thread_no)
            return mid;
        else if (current_no < thread_no)
            low = mid + 1;
        else
            high = mid - 1;
    } while (low < high);

    return low;
}

static unsigned thread_map_find(L4_thread_id global_id)
{
    unsigned idx = thread_list_find(global_id);
    if (!L4_same_threads(tcb_store[thread_list[idx]].global_id, global_id))
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

static struct tcb_t *allocate_tcb()
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

static void free_tcb(struct tcb_t *tcb)
{
    const unsigned idx = tcb - tcb_store;
    tcb_store_allocated &= ~(unsigned)(1 << idx);
}

static void thread_list_insert(unsigned tcb_idx, L4_thread_id global_id)
{
    unsigned idx = thread_list_find(global_id);
    // Assert the thread is not already present
    kassert(thread_count <= idx ||
            tcb_store[thread_list[idx]].global_id != global_id);
    ++thread_count;
    for (unsigned i = idx; i < thread_count; ++i)
    {
        const unsigned tmp = thread_list[i];
        thread_list[i] = tcb_idx;
        tcb_idx = tmp;
    }
}

static void thread_list_delete(L4_thread_id global_id)
{
    unsigned idx = thread_list_find(global_id);
    --thread_count;
    for (unsigned i = idx; i < thread_count; ++i)
        thread_list[i] = thread_list[i] + 1;
}

static void write_utcb(struct tcb_t *tcb)
{
    tcb->utcb->global_id = tcb->global_id;
    tcb->utcb->processor_no = 0;
    tcb->utcb->t_pager = tcb->pager;
    tcb->utcb->error = 0;
}

struct tcb_t *create_thread(L4_thread_id global_id)
{
    kassert(L4_version(global_id) != 0);
    if (thread_count == THREAD_MAX_COUNT)
        return NULL;
    struct tcb_t *tcb = allocate_tcb();
    if (!tcb)
        return NULL;
    tcb->global_id = global_id;
    tcb->local_id = 0;
    tcb->pager = L4_NILTHREAD;
    tcb->scheduler = L4_NILTHREAD;
    tcb->as = NULL;
    tcb->priority = 42; // TODO
    tcb->state = TS_INACTIVE;
    tcb->utcb = NULL;
    thread_list_insert(tcb - tcb_store, global_id);
    return tcb;
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
    update_kalarm_event(KALARM_RESCHEDULE, get_current_time() + 10);
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

void set_thread_priority(struct tcb_t *thread, unsigned prio)
{
    thread->priority = prio;
    // Update scheduler heap
    const unsigned idx = thread - tcb_store;
    unsigned heap_idx = find_idx_in_heap(idx);
    if (heap_idx == thread_schedule_fail)
        return;
    thread_schedule_swim(thread_schedule_sink(heap_idx));
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
    dbg_log(DBG_INTERRUPT, "Executing pend_sv\n");
    struct thread_context_t *tcb_ctx = get_current_ctx();
    if (tcb_ctx != NULL)
        *tcb_ctx = *current_context;
    schedule_next_thread();
    dbg_log(DBG_SCHEDULE, "Switched to thread #%x\n",
            get_current_thread()->global_id);
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
        "bx lr\n\t"
        ".pool\n\t");
    // TODO: Pass in functions as input operands. GCC is really doing anything
    // in its power not to accept it.
}

void start_scheduling()
{
    request_reschedule(NULL);
    while (1)
        asm volatile("wfi");
}

extern struct tcb_t *caller;

static bool fpage_contains_object(L4_fpage_t page, void *object_base,
                                  size_t object_size)
{
    return L4_address(page) <= (unsigned)object_base &&
           L4_address(page) + L4_size(page) >
               (unsigned)object_base + object_size;
}

static void syscall_thread_control_delete(unsigned *sp, struct tcb_t *dest_tcb)
{
    if (dest_tcb == get_root_tcb())
    {
        panic("The root thread exited!");
    }
    // Remove thread from scheduler heap, if present
    if (dest_tcb->state == TS_RUNNABLE)
    {
        unsigned heap_idx = find_idx_in_heap(dest_tcb - tcb_store);
        if (heap_idx != thread_schedule_fail)
            thread_schedule_delete(heap_idx);
    }
    // Delete from thread_list
    thread_list_delete(dest_tcb->global_id);
    if (dest_tcb->next_sibling)
        dest_tcb->next_sibling->prev_sibling = dest_tcb->prev_sibling;
    if (dest_tcb->prev_sibling)
        dest_tcb->prev_sibling->next_sibling = dest_tcb->next_sibling;
    free_tcb(dest_tcb);

    sp[THREAD_CTX_STACK_R0] = 1;
}

static void syscall_thread_control_modify(unsigned *sp, struct tcb_t *dest_tcb,
                                          L4_thread_id dest,
                                          L4_thread_id space_specifier,
                                          L4_thread_id scheduler,
                                          L4_thread_id pager,
                                          void *utcb_location)
{
    struct tcb_t *space_specifier_tcb =
        find_thread_by_global_id(space_specifier);
    struct tcb_t *scheduler_tcb = NULL;
    if (!space_specifier_tcb)
    {
        sp[THREAD_CTX_STACK_R0] = 0;
        caller->utcb->error = L4_error_invalid_space;
        return;
    }

    if (!L4_is_nil_thread(scheduler))
    {
        scheduler_tcb = find_thread_by_global_id(scheduler);
        if (scheduler_tcb == NULL)
        {
            sp[THREAD_CTX_STACK_R0] = 0;
            caller->utcb->error = L4_error_invalid_scheduler;
            return;
        }
    }

    // Make sure to check whether the UTCB location is valid in the potentially
    // new address space, not in the current one
    L4_fpage_t utcb_fpage = space_specifier_tcb->as->utcb_page;
    if (!fpage_contains_object(utcb_fpage, utcb_location, 160))
    {
        sp[THREAD_CTX_STACK_R0] = 0;
        caller->utcb->error = L4_error_utcb_area;
        return;
    }

    /* NOTE: We do not check for the existence of the pager thread, because
     * according the L4 spec, specifiying an invalid pager thread should not
     * cause an error (at least there is none given). I guess this is fine
     * as the thread will never start anyway without a pager thread that can
     * send it the start message.
     */

    // All checks have passed, modify the tcb
    dest_tcb->global_id = dest;
    dest_tcb->as = space_specifier_tcb->as;
    if (scheduler_tcb)
        dest_tcb->scheduler = scheduler;
    if (!L4_same_threads(pager, L4_NILTHREAD))
        dest_tcb->pager = pager;
    if ((unsigned)utcb_location != (unsigned)-1)
        dest_tcb->utcb = utcb_location;
    write_utcb(dest_tcb);
    sp[THREAD_CTX_STACK_R0] = 1;
}

static void syscall_thread_control_create(unsigned *sp, L4_thread_id dest,
                                          L4_thread_id space_control,
                                          L4_thread_id scheduler,
                                          L4_thread_id pager,
                                          void *utcb_location)
{
    // WARN: There are definitely bugs here. Like creating a thread in a new
    // address space will not work. Debug this when it's actually used
    // somewhere.
    struct tcb_t *space_control_tcb = find_thread_by_global_id(space_control);
    if (space_control_tcb == NULL)
    {
        sp[THREAD_CTX_STACK_R0] = 0;
        caller->utcb->error = L4_error_invalid_space;
        return;
    }
    if (space_control_tcb->as == NULL)
    {
        sp[THREAD_CTX_STACK_R0] = 0;
        caller->utcb->error = L4_error_invalid_space;
        return;
    }
    struct tcb_t *scheduler_tcb = NULL;
    if (!L4_is_nil_thread(scheduler))
    {
        scheduler_tcb = find_thread_by_global_id(scheduler);
        if (!scheduler_tcb)
        {
            sp[THREAD_CTX_STACK_R0] = 0;
            caller->utcb->error = L4_error_invalid_scheduler;
            return;
        }
    }
    if (!fpage_contains_object(space_control_tcb->as->utcb_page, utcb_location,
                               160))
    {
        sp[THREAD_CTX_STACK_R0] = 0;
        caller->utcb->error = L4_error_kip_area;
        return;
    }

    struct tcb_t *tcb = create_thread(dest);
    tcb->global_id = dest;
    tcb->local_id = (L4_thread_id)&utcb_location;
    tcb->state = TS_INACTIVE;
    tcb->as = space_control_tcb->as;
    tcb->pager = pager;
    tcb->scheduler = scheduler;
    if ((unsigned)utcb_location != (unsigned)-1)
    {
        tcb->utcb = utcb_location;
    }

    sp[THREAD_CTX_STACK_R0] = 1;
}

void syscall_thread_control()
{
    unsigned *const sp = (unsigned *)caller->ctx.sp;
    const L4_thread_id dest = sp[THREAD_CTX_STACK_R0];
    const L4_thread_id space_specifier = sp[THREAD_CTX_STACK_R1];
    const L4_thread_id scheduler = sp[THREAD_CTX_STACK_R2];
    const L4_thread_id pager = sp[THREAD_CTX_STACK_R3];
    void *utcb_location = (void *)caller->ctx.r[THREAD_CTX_R4];

    struct tcb_t *const dest_tcb = find_thread_by_global_id(dest);
    if (dest_tcb)
    {
        if (L4_same_threads(space_specifier, L4_NILTHREAD))
        {
            // dest exists, space_specifier == nilthread => Delete dest
            syscall_thread_control_delete(sp, dest_tcb);
        }
        else
        {
            // dest exists, space_specifier != nilthread => Modify dest
            syscall_thread_control_modify(sp, dest_tcb, dest, space_specifier,
                                          scheduler, pager, utcb_location);
        }
    }
    else
    {
        if (L4_same_threads(space_specifier, L4_NILTHREAD))
        {
            sp[THREAD_CTX_STACK_R0] = 0;
            caller->utcb->error = L4_error_invalid_space;
        }
        else
        {
            syscall_thread_control_create(sp, dest, space_specifier, scheduler,
                                          pager, utcb_location);
        }
    }
}
