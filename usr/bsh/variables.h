#ifndef BRADYPOS_BSH_VARIABLES_H
#define BRADYPOS_BSH_VARIABLES_H

enum
{
    IPC_BUFFER_SIZE = 128,
};

extern unsigned char ipc_buffer[IPC_BUFFER_SIZE];
extern L4_thread_id term_service;
extern L4_thread_id romfs_service;
extern int last_exit_code;

#endif
