/**
 * @file clock.h
 * @brief Clock HAL implementation for Cortex-M4 (STM32F401RE).
 *
 * This header provides register definitions, configuration structures,
 * and function declarations to initialize and retrieve system and bus clocks
 * on Cortex-M4 microcontrollers, specifically STM32F401RE in this
 * implementation.
 *
 * It supports configuring the system clock source including HSI, HSE, and PLL,
 * PLL parameter setup, and clock prescaler controls for AHB and APB buses.
 *
 * @ingroup HAL_CLOCK
 *
 * @note This file is architecture-specific and included through the common
 * `hal_clock.h` dispatcher based on target definitions (e.g., `CORTEX_M4`).
 *
 * @author Ashutosh Vishwakarma
 * @date 2025-07-21
 */

#ifndef CORTEX_M4_CLOCK_H
#define CORTEX_M4_CLOCK_H

#include "utils/clock_types.h"

/**
 * @brief PLL configuration parameters.
 *
 * Used to configure the PLL source and multiplication/division factors.
 */
typedef struct {
  hal_clock_source_t input_src; ///< PLL input clock source (HSI or HSE)
  uint8_t pll_m;  ///< PLL division factor for input clock (2 to 63)
  uint16_t pll_n; ///< PLL multiplication factor for VCO (50 to 432)
  uint8_t pll_p;  ///< PLL division factor for main system clock (2,4,6,8)
  uint8_t pll_q;  ///< PLL division factor for USB clock (2 to 15)
} hal_pll_config_t;

/** @defgroup RCC_BASE RCC Peripheral Base and Register Offsets
 * @{
 */


/** @} */

/**
 * @brief Initialize system clock with given configuration.
 *
 * Configures the system clock source (HSI, HSE, PLL), PLL parameters,
 * flash latency, and bus prescalers (AHB, APB1, APB2).
 *
 * @param cfg Pointer to clock configuration structure (source selection).
 * @param pll_cfg Pointer to PLL configuration structure if PLL is used.
 */
void hal_clock_init(hal_clock_config_t *cfg, hal_pll_config_t *pll_cfg);

/**
 * @brief Get the current system clock frequency (SYSCLK) in Hz.
 *
 * Reads RCC registers and calculates SYSCLK frequency based on
 * current clock source and PLL settings.
 *
 * @return System clock frequency in Hertz.
 */
uint32_t hal_clock_get_sysclk(void);

/**
 * @brief Get the current AHB bus clock frequency in Hz.
 *
 * Calculated from SYSCLK frequency divided by AHB prescaler.
 *
 * @return AHB bus clock frequency in Hertz.
 */
uint32_t hal_clock_get_ahbclk(void);

/**
 * @brief Get the current APB1 bus clock frequency in Hz.
 *
 * Calculated from SYSCLK frequency divided by APB1 prescaler.
 *
 * @return APB1 bus clock frequency in Hertz.
 */
uint32_t hal_clock_get_apb1clk(void);

/**
 * @brief Get the current APB2 bus clock frequency in Hz.
 *
 * Calculated from SYSCLK frequency divided by APB2 prescaler.
 *
 * @return APB2 bus clock frequency in Hertz.
 */
uint32_t hal_clock_get_apb2clk(void);

#endif // CORTEX_M4_CLOCK_H
