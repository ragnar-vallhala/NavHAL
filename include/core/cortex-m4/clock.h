/**
 * @file clock.h
 * @brief Clock HAL implementation for Cortex-M4 (STM32F401RE).
 *
 * This header provides register definitions, configuration structures,
 * and function declarations to initialize and retrieve system and bus clocks
 * on Cortex-M4 microcontrollers, specifically STM32F401RE in this implementation.
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

#include "utils/types.h"

/**
 * @brief PLL configuration parameters.
 *
 * Used to configure the PLL source and multiplication/division factors.
 */
typedef struct
{
    hal_clock_source_t input_src; ///< PLL input clock source (HSI or HSE)
    uint8_t pll_m;                ///< PLL division factor for input clock (2 to 63)
    uint16_t pll_n;               ///< PLL multiplication factor for VCO (50 to 432)
    uint8_t pll_p;                ///< PLL division factor for main system clock (2,4,6,8)
    uint8_t pll_q;                ///< PLL division factor for USB clock (2 to 15)
} hal_pll_config_t;

/** @defgroup RCC_BASE RCC Peripheral Base and Register Offsets
 * @{
 */
#define RCC 0x40023800UL        ///< RCC base address
#define RCC_CR_OFFSET 0x00      ///< Clock control register offset
#define RCC_PLLCFGR_OFFSET 0x04 ///< PLL configuration register offset
#define RCC_CFGR_OFFSET 0x08    ///< Clock configuration register offset

// RCC_CR register bit positions
#define RCC_CR_HSE_ON_BIT 16    ///< HSE clock enable bit
#define RCC_CR_HSE_READY_BIT 17 ///< HSE clock ready flag bit
#define RCC_CR_HSI_ON_BIT 0     ///< HSI clock enable bit
#define RCC_CR_HSI_READY_BIT 1  ///< HSI clock ready flag bit
#define RCC_CR_PLL_ON_BIT 24    ///< PLL enable bit
#define RCC_CR_PLL_READY_BIT 25 ///< PLL ready flag bit

// RCC_PLLCFGR register bit positions
#define RCC_PLLCFGR_SRC_BIT 22  ///< PLL source selection bit
#define RCC_PLLCFGR_PLLM_BIT 0  ///< PLLM bits (6 bits)
#define RCC_PLLCFGR_PLLN_BIT 6  ///< PLLN bits (9 bits)
#define RCC_PLLCFGR_PLLP_BIT 16 ///< PLLP bits (2 bits)
#define RCC_PLLCFGR_PLLQ_BIT 24 ///< PLLQ bits (4 bits)

// RCC_CFGR register bit positions
#define RCC_CFGR_HPRE_BIT 4   ///< AHB prescaler bits (4 bits)
#define RCC_CFGR_PPRE1_BIT 10 ///< APB1 prescaler bits (3 bits)
#define RCC_CFGR_PPRE2_BIT 13 ///< APB2 prescaler bits (3 bits)
#define RCC_CFGR_SW_BIT 0     ///< System clock switch bits (2 bits)
#define RCC_CFGR_SWS_BIT 2    ///< System clock switch status bits (2 bits)

// Prescaler values
#define RCC_CFGR_HPRE_DIV1 0x0  ///< AHB clock: SYSCLK not divided
#define RCC_CFGR_PPRE1_DIV4 0x5 ///< APB1 clock: AHB divided by 4
#define RCC_CFGR_PPRE2_DIV2 0x4 ///< APB2 clock: AHB divided by 2

// Flash Interface Control
#define FLASH_INTERFACE_REGISTER 0x40023C00 ///< Flash Interface base address
#define FLASH_ACR_LATENCY_BIT 0             ///< Flash ACR Latency bits

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
uint32_t hal_clock_get_sysclk();

/**
 * @brief Get the current AHB bus clock frequency in Hz.
 *
 * Calculated from SYSCLK frequency divided by AHB prescaler.
 *
 * @return AHB bus clock frequency in Hertz.
 */
uint32_t hal_clock_get_ahbclk();

/**
 * @brief Get the current APB1 bus clock frequency in Hz.
 *
 * Calculated from SYSCLK frequency divided by APB1 prescaler.
 *
 * @return APB1 bus clock frequency in Hertz.
 */
uint32_t hal_clock_get_apb1clk();

/**
 * @brief Get the current APB2 bus clock frequency in Hz.
 *
 * Calculated from SYSCLK frequency divided by APB2 prescaler.
 *
 * @return APB2 bus clock frequency in Hertz.
 */
uint32_t hal_clock_get_apb2clk();

#endif // CORTEX_M4_CLOCK_H
