#include <l4/kip.h>
#include <stddef.h>

__attribute__((naked)) void _start() { __asm__("b main\n\t"); }

static kip_t *the_kip;

int main() { L4_kernel_interface(NULL, NULL, NULL); }
