#ifndef BRADYPOS_ROOT_VARIABLES_H
#define BRADYPOS_ROOT_VARIABLES_H

#include <l4/kip.h>
#include <l4/thread.h>

extern L4_kip_t *the_kip;
extern L4_thread_id my_thread_id;
extern L4_thread_id romfs_thread_id;

#endif