#include <semihosting.h>
#include <string.h>

unsigned trap_sh(enum semihosting_syscall nr, uintptr_t ptr);

int sh_open(const char *path, int opening_mode)
{
    return sh_open_s(path, opening_mode, strlen(path));
}

int sh_open_s(const char *path, int opening_mode, size_t size)
{
    struct
    {
        const char *path;
        int opening_mode;
        size_t size;
    } param = {.path = path, .opening_mode = opening_mode, .size = size};
    return trap_sh(SYS_OPEN, (uintptr_t)&param);
}

int sh_close(int fd)
{
    struct
    {
        int fd;
    } param = {.fd = fd};
    return trap_sh(SYS_CLOSE, (uintptr_t)&param);
}

void sh_writec(char c) { (void)trap_sh(SYS_WRITEC, (uintptr_t)&c); }
void sh_write0(const char *str) { (void)trap_sh(SYS_WRITE0, (uintptr_t)str); }
size_t sh_write(int fd, const void *data, size_t len)
{
    struct
    {
        int fd;
        const void *data;
        size_t len;
    } param = {.fd = fd, .data = data, .len = len};
    return (size_t)trap_sh(SYS_WRITE, (uintptr_t)&param);
}

size_t sh_read(int fd, void *data, size_t len)
{
    struct
    {
        int fd;
        void *data;
        size_t len;
    } param = {.fd = fd, .data = data, .len = len};
    return (size_t)trap_sh(SYS_READ, (uintptr_t)&param);
}
char sh_readc() { return trap_sh(SYS_READC, (uintptr_t)0); }

int sh_iserror(size_t return_code)
{
    return trap_sh(SYS_ISERROR, (uintptr_t)return_code);
}

int sh_seek(int fd, size_t offset)
{
    struct
    {
        int fd;
        size_t offset;
    } param = {.fd = fd, .offset = offset};
    return (int)trap_sh(SYS_SEEK, (uintptr_t)&param);
}

int sh_tmpnam(char *buf, int id, size_t size)
{
    struct
    {
        char *buf;
        int id;
        size_t size;
    } param = {.buf = buf, .id = id, .size = size};
    return (int)trap_sh(SYS_TMPNAM, (uintptr_t)&param);
}

int sh_remove(const char *path, size_t len)
{
    struct
    {
        const char *path;
        size_t len;
    } param = {.path = path, .len = len};
    return (int)trap_sh(SYS_REMOVE, (uintptr_t)&param);
}

int sh_rename(const char *old_name, size_t old_len, const char *new_name,
              size_t new_len)
{
    struct
    {
        const char *old_name;
        size_t old_len;
        const char *new_name;
        size_t new_len;
    } param = {.old_name = old_name,
               .old_len = old_len,
               .new_name = new_name,
               .new_len = new_len};
    return (int)trap_sh(SYS_RENAME, (uintptr_t)&param);
}

int sh_clock() { return (int)trap_sh(SYS_CLOCK, (uintptr_t)0); }
int sh_time() { return (int)trap_sh(SYS_TIME, (uintptr_t)0); }
int sh_system(const char *cmd, size_t len)
{
    struct
    {
        const char *cmd;
        size_t len;
    } param = {.cmd = cmd, .len = len};
    return (int)trap_sh(SYS_SYSTEM, (uintptr_t)&param);
}
int sh_errno() { return (int)trap_sh(SYS_ERRNO, (uintptr_t)0); }

struct sh_heapinfo_t *sh_heapinfo(struct sh_heapinfo_t *info)
{
    register const unsigned r0 asm("r0") = SYS_HEAPINFO;
    register uintptr_t r1 asm("r1") = (uintptr_t)&info;
    __asm__ volatile("bkpt 0xab\n" : "=r"(r1) : "r"(r0), "0"(r1));
    return *(struct sh_heapinfo_t **)r1;
}

void sh_report_exception(enum sh_exception_type type)
{
    (void)trap_sh(SYS_ReportException, (uintptr_t)type);
}
