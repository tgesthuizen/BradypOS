#ifndef BRADYPOS_BSH_FILEIO_H
#define BRADYPOS_BSH_FILEIO_H

#include <l4/thread.h>
#include <stddef.h>

enum
{
    ROOT_FD = 3,
    ETC_FD = 4,
    MOTD_FD = 5,
    VERSION_FD = 6,
    WD_FD = 7,
    TEMP1_FD = 8,
    TEMP2_FD = 9,
};

bool term_write(L4_thread_id term_service, const unsigned char *buf,
                size_t size);
int getch();
void ungetch(int ch);

#endif
