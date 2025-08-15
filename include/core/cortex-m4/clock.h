/**
 * @file core/cortex-m4/clock.h
 * @brief Cortex-M4 specific clock control HAL interface.
 *
 * @details
 * This header defines the structures and functions to configure and query
 * the system and peripheral clocks for Cortex-M4 microcontrollers.
 * It includes PLL configuration, system clock initialization, and AHB/APB
 * clock retrieval functions.
 *
 * The HAL provides a platform-independent abstraction while leveraging
 * the Cortex-M4 specific hardware.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_CLOCK_H
#define CORTEX_M4_CLOCK_H

#include "utils/clock_types.h"

/**
 * @brief PLL (Phase Locked Loop) configuration structure.
 *
 * @details
 * This structure defines the parameters for configuring the PLL to generate
 * the system clock from the selected input source.
 */
typedef struct {
    hal_clock_source_t input_src; /**< Clock input source for PLL. */
    uint8_t pll_m;                /**< Division factor for PLL input. */
    uint16_t pll_n;               /**< Multiplication factor for PLL VCO. */
    uint8_t pll_p;                /**< Division factor for main system clock. */
    uint8_t pll_q;                /**< Division factor for peripheral clocks. */
} hal_pll_config_t;

/**
 * @brief Initialize the system clock.
 *
 * @param cfg Pointer to the main clock configuration structure.
 * @param pll_cfg Pointer to the PLL configuration structure.
 *
 * @note This function must be called before using other peripheral clocks.
 */
void hal_clock_init(hal_clock_config_t *cfg, hal_pll_config_t *pll_cfg);

/**
 * @brief Get the system clock frequency (SYSCLK).
 *
 * @return System clock frequency in Hz.
 */
uint32_t hal_clock_get_sysclk(void);

/**
 * @brief Get the AHB bus clock frequency.
 *
 * @return AHB clock frequency in Hz.
 */
uint32_t hal_clock_get_ahbclk(void);

/**
 * @brief Get the APB1 bus clock frequency.
 *
 * @return APB1 clock frequency in Hz.
 */
uint32_t hal_clock_get_apb1clk(void);

/**
 * @brief Get the APB2 bus clock frequency.
 *
 * @return APB2 clock frequency in Hz.
 */
uint32_t hal_clock_get_apb2clk(void);

#endif // CORTEX_M4_CLOCK_H
