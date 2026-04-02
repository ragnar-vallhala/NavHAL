/**
 * @file dwt.c
 * @brief Cortex-M4 DWT implementation.
 *
 * @details
 * This module provides the implementation of the DWT API for high-resolution
 * cycle counting and timing.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "core/cortex-m4/dwt.h"
#include "core/cortex-m4/dwt_reg.h"

/**
 * @brief Initialize the DWT unit.
 *
 * @details
 * Enables the trace unit (TRCENA in DEMCR) and starts the cycle counter
 * (CYCCNTENA in DWT_CTRL).
 */
void dwt_init(void) {
  // 1. Enable CoreDebug TRCENA
  CoreDebug->DEMCR |= CORE_DEBUG_DEMCR_TRCENA_BIT;

  // 2. Clear cycle counter
  DWT->CYCCNT = 0;

  // 3. Enable CYCCNTENA
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_BIT;
}

/**
 * @brief Get the current cycle count.
 *
 * @return 32-bit cycle count (CYCCNT).
 */
uint32_t dwt_get_cycles(void) { return DWT->CYCCNT; }

/**
 * @brief Reset the cycle counter to zero.
 */
void dwt_reset_cycles(void) { DWT->CYCCNT = 0; }

/**
 * @brief Block for a specified number of processor cycles.
 *
 * @param cycles Number of cycles to delay.
 */
void dwt_delay_cycles(uint32_t cycles) {
  uint32_t start = dwt_get_cycles();
  while ((dwt_get_cycles() - start) < cycles) {
    __asm__ volatile("nop");
  }
}
