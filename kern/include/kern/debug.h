#ifndef BRADYPOS_KERN_DEBUG_H
#define BRADYPOS_KERN_DEBUG_H

void dbg_puts(const char *str);
void dbg_printf(const char *fmt, ...);

#ifndef NDEBUG
#define kassert(cond)                                                          \
    ((cond) ? panic("%s:%d: kassert(%s) failed!\n", __FILE__, __LINE__, #cond) \
            : (void)0)
#else
#define kassert(cond) ((void)0)
#endif

__attribute__((noreturn)) void panic(const char *fmt, ...);
__attribute__((noreturn)) void reset_processor();
#endif
