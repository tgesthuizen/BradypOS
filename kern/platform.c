#include <kern/platform.h>

void enable_interrupts();
void disable_interrupts();
void dsb();
void isb();
unsigned *get_msp();
unsigned *get_psp();
