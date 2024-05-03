#include <kern/debug.h>
#include <kern/systhread.h>

static struct tcb_t *kern_tcb;
static struct tcb_t *root_tcb;
static L4_thread_t kern_id;
static L4_thread_t root_id;

void create_sys_threads() {}

void switch_to_kernel() { panic("kernel switch not yet implemented\n"); }
struct tcb_t *get_kernel_tcb() { return kern_tcb; }
