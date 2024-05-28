#ifndef L4_UTCB_H
#define L4_UTCB_H

#include <l4/thread.h>

// These symbols are not standardized
enum
{
    L4_MR_COUNT = 16,
    L4_BR_COUNT = 8,
};

struct L4_utcb
{
    L4_thread_id global_id;
    unsigned processor_no;
    unsigned user_defined_handle;
    L4_thread_id t_pager;
    unsigned exception_handler;
    unsigned flags;
    unsigned error;

    L4_thread_id intended_receiver;
    L4_thread_id sender;
    unsigned thread_word_1;
    unsigned thread_word_2;
    unsigned mr[16];
    unsigned br[8];
};

typedef struct L4_utcb L4_utcb_t;

#define L4_UTCB_SIZE (sizeof(struct L4_utcb_t))

#endif
