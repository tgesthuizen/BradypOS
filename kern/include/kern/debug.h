#ifndef BRADYPOS_KERN_DEBUG_H
#define BRADYPOS_KERN_DEBUG_H

void dbg_puts(const char *str);
void dbg_printf(const char *fmt, ...);

__attribute__((noreturn)) void panic(const char *fmt, ...);
__attribute__((noreturn)) void reset_processor();
#endif
