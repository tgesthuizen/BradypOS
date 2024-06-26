#ifndef BRADYPOS_SEMIHOSTING_H
#define BRADYPOS_SEMIHOSTING_H

#include <stddef.h>
#include <stdint.h>

enum semihosting_syscall
{
    SYS_OPEN = 0x01,
    SYS_CLOSE,
    SYS_WRITEC,
    SYS_WRITE0,
    SYS_WRITE,
    SYS_READ,
    SYS_READC,
    SYS_ISERROR,
    SYS_ISTTY,
    SYS_SEEK,
    SYS_FLEN = 0x0C,
    SYS_TMPNAM,
    SYS_REMOVE,
    SYS_RENAME,
    SYS_CLOCK,
    SYS_TIME,
    SYS_SYSTEM,
    SYS_ERRNO,
    SYS_GET_CMDLINE = 0x15,
    SYS_HEAPINFO,
    SYS_EnterSVC,
    SYS_ReportException,

    SYS_ELAPSED = 0x30,
    SYS_TICKFREQ = 0x31,

};

inline unsigned trap_sh(enum semihosting_syscall nr, uintptr_t ptr)
{
    register unsigned r0 asm("r0") = nr;
    register uintptr_t r1 asm("r1") = ptr;
    __asm__ volatile("bkpt 0xab\n" : "=r"(r0) : "0"(r0), "r"(r1) : "memory");
    return r0;
}

int sh_open(const char *path, int opening_mode);
int sh_open_s(const char *path, int opening_mode, size_t size);
int sh_close(int fd);
void sh_writec(char c);
void sh_write0(const char *str);
size_t sh_write(int fd, const void *data, size_t len);
size_t sh_read(int fd, void *data, size_t len);
char sh_readc();
int sh_iserror(size_t return_code);
int sh_seek(int fd, size_t offset);
int sh_tmpnam(char *buf, int id, size_t size);
int sh_remove(const char *path, size_t len);
int sh_rename(const char *old_name, size_t old_len, const char *new_name,
              size_t new_len);
int sh_clock();
int sh_time();
int sh_system(const char *cmd, size_t len);
int sh_errno();

struct sh_heapinfo_t
{
    int heap_base;
    int heap_size;
    int stack_base;
    int stack_limit;
};
struct sh_heapinfo_t *sh_heapinfo(struct sh_heapinfo_t *info);

enum sh_exception_type
{
    ADP_Stopped_BranchThroughZero = 0x20000,
    ADP_Stopped_UndefinedInstr,
    ADP_Stopped_SoftwareInterrupt,
    ADP_Stopped_PrefetchAbort,
    ADP_Stopped_DataAbort,
    ADP_Stopped_AddressException,
    ADP_Stopped_IRQ,
    ADP_Stopped_FIQ,
    ADP_Stopped_BreakPoint = 0x20020,
    ADP_Stopped_WatchPoint,
    ADP_Stopped_StepComplete,
    ADP_Stopped_RunTimeErrorUnknown,
    ADP_Stopped_InternalError,
    ADP_Stopped_UserInterruption,
    ADP_Stopped_ApplicationExit,
    ADP_Stopped_StackOverflow,
    ADP_Stopped_DivisionByZero,
    ADP_Stopped_OSSpecific,
};

void sh_report_exception(enum sh_exception_type type);

#endif
