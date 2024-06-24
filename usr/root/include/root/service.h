#ifndef BRADYPOS_ROOT_SERVICE_H
#define BRADYPOS_ROOT_SERVICE_H

#include <l4/ipc.h>
#include <l4/thread.h>

void make_service_ipc_error(int errno);

L4_thread_id get_service(unsigned name);
void register_service(L4_thread_id tid, unsigned name);
void register_service_from_name(L4_thread_id tid, char *name);

void handle_service_get(L4_msg_tag_t tag, L4_thread_id from);
void handle_service_register(L4_msg_tag_t tag, L4_thread_id from);

#endif
