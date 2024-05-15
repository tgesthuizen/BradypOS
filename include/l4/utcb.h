#ifndef L4_UTCB_H
#define L4_UTCB_H

#include <l4/thread.h>
#include <stdint.h>

struct L4_utcb
{
    L4_thread_id global_id;
    uint32_t processor_no;
    uint32_t user_defined_handle;
    L4_thread_id t_pager;
    uint32_t exception_handler;
    uint32_t flags;

    L4_thread_id intended_receiver;
    L4_thread_id sender;
    uint32_t thread_word_1;
    uint32_t thread_word_2;
    uint32_t mr[16];
    uint32_t br[8];
};

typedef struct L4_utcb L4_utcb_t;

#define L4_UTCB_SIZE (sizeof(struct L4_utcb_t))

#endif
