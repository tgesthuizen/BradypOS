#ifndef BRADYPOS_ROOT_VARIABLES_H
#define BRADYPOS_ROOT_VARIABLES_H

#include <l4/kip.h>
#include <l4/thread.h>
#include <l4/utcb.h>

extern L4_kip_t *the_kip;
extern L4_thread_id my_thread_id;
extern L4_thread_id romfs_thread_id;
extern L4_utcb_t __utcb;
extern L4_utcb_t *romfs_server_utcb;
extern unsigned next_thread_no;

#endif
