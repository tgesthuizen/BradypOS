static int main();

__attribute__((naked)) void _start() {
  asm volatile("movs r0, #0\n\t"
               "movs lr, r0\n\t"
               "bl [main]\n\t"
               "b ."
               :
               : [main] "m"(main)
               : "r0", "lr");
}

extern void *romfs_start;

static int main() {}
