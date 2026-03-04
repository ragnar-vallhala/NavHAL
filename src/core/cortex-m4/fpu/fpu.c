#include "core/cortex-m4/fpu.h"
#include <stdint.h>
#define CPACR (*(volatile uint32_t *)0xE000ED88)
#define FPCCR (*(volatile uint32_t *)0xE000EF34)

#ifdef _FPU_ENABLED
void hal_fpu_enable(void) {
  // CPACR: Enable full access to CP10 and CP11
  CPACR |= (0xF << 20);

  // FPCCR: Enable lazy stacking (ASPEN and LSPEN bits)
  FPCCR |= (0x3 << 30);

  // Ensure all pipeline operations are complete
  __asm volatile("dsb");
  __asm volatile("isb");
}
#else
void hal_fpu_enable(void) {}
#endif
