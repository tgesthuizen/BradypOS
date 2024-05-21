#ifndef L4_THREAD_H
#define L4_THREAD_H

#include <l4/syscalls.h>
#include <stdbool.h>

typedef unsigned L4_thread_id;

#define L4_NILTHREAD (0)
#define L4_ANYTHREAD (0xFFFFFFFF)
#define L4_ANYLOCALTHREAD (0xFFFFFFB0)

inline L4_thread_id L4_global_id(unsigned number, unsigned version)
{
    return (number << 14) | version;
}
inline unsigned L4_version(L4_thread_id tid) { return tid & ((1 << 15) - 1); }
inline unsigned L4_thread_no(L4_thread_id tid) { return tid >> 14; }
inline bool L4_same_threads(L4_thread_id lhs, L4_thread_id rhs)
{
    return L4_thread_no(lhs) == L4_thread_no(rhs);
}
inline bool L4_is_nil_thread(L4_thread_id tid) { return tid == L4_NILTHREAD; }
inline bool L4_is_local_id(L4_thread_id tid)
{
    return (tid & ((1 << 7) - 1)) == 0;
}
inline bool L4_is_global_id(L4_thread_id tid)
{
    return (tid & ((1 << 7) - 1)) != 0;
}
L4_thread_id L4_local_id_of(L4_thread_id tid);
L4_thread_id L4_global_id_of(L4_thread_id tid);
L4_thread_id L4_my_global_id();
L4_thread_id L4_my_local_id();

inline unsigned L4_thread_control(L4_thread_id dest,
                                  L4_thread_id space_specifier,
                                  L4_thread_id scheduler, L4_thread_id pager,
                                  void *utcb_location)
{
    register L4_thread_id rdest asm("r0") = dest;
    register L4_thread_id rss asm("r1") = space_specifier;
    register L4_thread_id rs asm("r2") = scheduler;
    register L4_thread_id rp asm("r3") = pager;
    register void *rul asm("r4") = utcb_location;
    unsigned ret;
    asm volatile("movs r7, %[SYS_THREAD_CONTROL]\n\t"
                 "svc #0\n\t"
                 : "=r"(ret)
                 : [SYS_THREAD_CONTROL] "i"(SYS_THREAD_CONTROL), "0"(rdest),
                   "r"(rss), "r"(rs), "r"(rp), "r"(rul)
                 : "r7");
    return ret;
}

#endif
