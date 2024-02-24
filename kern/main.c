#include "debug.h"
#include "platform.h"
#include <stdint.h>

int main() {
  *(volatile uintptr_t *)(PPB_BASE + VTOR_OFFSET) = (uintptr_t)__vector;
  enable_interrupts();

  dbg_puts("Interrupts are enabled, we have booted!\n");

  volatile uint32_t *const target_gpio =
      (volatile uint32_t *)(IO_BANK0_BASE + GPIO_BLOCK_SIZE * TARGET_GPIO);
  target_gpio[1] = PIN_FUNC_SIO | 3 << 12;
  *(volatile uint32_t *)(SIO_BASE + SIO_GPIO_OUT_SET) = 1 << 25;
}

void toggle_led() {
  *(volatile uint32_t *)(SIO_BASE + SIO_GPIO_OUT_XOR) = 1 << 25;
}
