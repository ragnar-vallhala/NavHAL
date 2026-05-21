/**
 * @file hal_clock.h
 * @brief Portable HAL interface for clock management.
 *
 * @details
 * Public API for system and peripheral clock control. PLL configuration and
 * the system-clock initialization function live here; bus-frequency queries
 * are also portable. Any target-specific extensions live in the port header
 * (@c port/cortex-m4/navhal_port_clock.h on the Cortex-M4 port), included at the bottom.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_CLOCK_H
#define HAL_CLOCK_H

#include "common/hal_status.h"
#include "utils/clock_types.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PLL (Phase Locked Loop) configuration structure.
 *
 * Defines the parameters used to derive the system clock from the selected
 * input source.
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
 * @param cfg     Main clock configuration; must not be NULL.
 * @param pll_cfg PLL configuration; must not be NULL when
 *                @c cfg->source is ::HAL_CLOCK_SOURCE_PLL, ignored otherwise.
 * @return ::HAL_OK on success, or ::HAL_ERR_INVALID_ARG if a required
 *         argument is NULL.
 *
 * @note Must be called before using other peripheral clocks.
 */
hal_status_t hal_clock_init(const hal_clock_config_t *cfg,
                            const hal_pll_config_t *pll_cfg);

/** @brief Get the system clock frequency (SYSCLK) in Hz. */
uint32_t hal_clock_get_sysclk(void);

/** @brief Get the AHB bus clock frequency in Hz. */
uint32_t hal_clock_get_ahbclk(void);

/** @brief Get the APB1 bus clock frequency in Hz. */
uint32_t hal_clock_get_apb1clk(void);

/** @brief Get the APB2 bus clock frequency in Hz. */
uint32_t hal_clock_get_apb2clk(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "navhal_port_clock.h"

#endif /* HAL_CLOCK_H */
