/**
 * @file fpu.c
 * @brief Standardized HAL hardware-FPU driver for Cortex-M4.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "navhal_port_fpu.h"
#include <stdint.h>

#define CPACR (*(volatile uint32_t *)0xE000ED88)
#define FPCCR (*(volatile uint32_t *)0xE000EF34)

#ifdef _FPU_ENABLED
hal_status_t hal_fpu_enable(void) {
  // CPACR: Enable full access to CP10 and CP11
  CPACR |= (0xF << 20);

  // FPCCR: Enable lazy stacking (ASPEN and LSPEN bits)
  FPCCR |= (0x3 << 30);

  // Ensure all pipeline operations are complete
  __asm volatile("dsb");
  __asm volatile("isb");
  return HAL_OK;
}
#else
hal_status_t hal_fpu_enable(void) { return HAL_ERR_NOT_SUPPORTED; }
#endif
