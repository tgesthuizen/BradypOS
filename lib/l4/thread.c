#include <l4/thread.h>
#include <l4/utcb.h>

L4_thread_id L4_global_id(unsigned number, unsigned version);
unsigned L4_version(L4_thread_id tid);
unsigned L4_thread_no(L4_thread_id tid);
bool L4_same_threads(L4_thread_id lhs, L4_thread_id rhs);
bool L4_is_nil_thread(L4_thread_id tid);
bool L4_is_local_id(L4_thread_id tid);
bool L4_is_global_id(L4_thread_id tid);

extern L4_utcb_t __utcb;

L4_thread_id L4_my_global_id() { return __utcb.global_id; }
L4_thread_id L4_my_local_id() { return *(unsigned *)&__utcb; }