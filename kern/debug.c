#include "debug.h"
#include "semihosting.h"

void dbg_puts(const char *str) { sh_write0(str); }
