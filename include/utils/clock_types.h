/**
 * @file clock_types.h
 * @brief Clock HAL type definitions.
 *
 * Defines clock source enumerations and clock configuration
 * structures used by the clock HAL for Cortex-M4 MCUs.
 *
 * @ingroup HAL_CLOCK
 *
 * @author Ashutosh Vishwakarma
 * @date 2025-07-21
 */

#ifndef CLOCK_TYPES_H
#define CLOCK_TYPES_H

#include "core/cortex-m4/rcc_reg.h"
#include <stdint.h>

/**
 * @brief Enumeration of possible system clock sources.
 */
typedef enum {
  HAL_CLOCK_SOURCE_HSI, ///< Internal high-speed oscillator (~16 MHz)
  HAL_CLOCK_SOURCE_HSE, ///< External high-speed oscillator (user-provided
                        ///< crystal)
  HAL_CLOCK_SOURCE_PLL  ///< Phase-locked loop (derived clock)
} hal_clock_source_t;

/**
 * @brief System clock configuration structure.
 *
 * Selects the clock source to be used as SYSCLK.
 */
typedef struct {
  hal_clock_source_t source; ///< Selected clock source (HSI, HSE, or PLL)
  rcc_cfgr_hpre_div_t hpre_div;
  rcc_cfgr_ppre_div_t ppre1_div;
  rcc_cfgr_ppre_div_t ppre2_div;
} hal_clock_config_t;

#endif // CLOCK_TYPES_H
