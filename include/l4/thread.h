#ifndef L4_THREAD_H
#define L4_THREAD_H

#include <stdbool.h>

typedef unsigned L4_thread_id;

#define L4_NILTHREAD (0)
#define L4_ANYTHREAD (0xFFFFFFFF)
#define L4_ANYLOCALTHREAD (0xFFFFFFB0)

static inline L4_thread_id L4_global_id(unsigned number, unsigned version)
{
    return (number << 6) | version;
}
static inline unsigned L4_version(L4_thread_id tid) { return tid & 0xF3; }
static inline unsigned L4_thread_no(L4_thread_id tid) { return tid >> 6; }
static inline bool L4_same_threads(L4_thread_id lhs, L4_thread_id rhs)
{
    return L4_thread_no(lhs) == L4_thread_no(rhs);
}
static inline bool L4_is_nil_thread(L4_thread_id tid) { return tid == L4_NILTHREAD; }

#endif
