#ifndef BRADYPOS_ROOT_CONFIG_H
#define BRADYPOS_ROOT_CONFIG_H

#include <l4/thread.h>
#include <stdbool.h>
#include <stddef.h>

void parse_init_config(L4_thread_id romfs_server);

#endif
