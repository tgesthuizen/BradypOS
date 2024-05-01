#ifndef L4_UTCB_H
#define L4_UTCB_H

#include <stdint.h>
#include <types.h>

struct L4_utcb
{
    uint32_t processor_no;
    uint32_t user_defined_handle;
    L4_thread_t t_pager;
    uint32_t exception_handler;
    uint32_t flags;

    L4_thread_t intended_receiver;
    L4_thread_t sender;
    uint32_t thread_word_1;
    uint32_t thread_word_2;
    uint32_t mr[8]; // Message registers 8-15, MR 0 - 7 are lying in r4..r11.
    uint32_t br[8];
};

typedef struct L4_utcb L4_utcb_t;

#define L4_UTCB_SIZE (sizeof(struct L4_utcb_t))

#endif
