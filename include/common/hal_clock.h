/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file hal_clock.h
 * @brief Portable HAL interface for clock management.
 *
 * @details
 * Public API for system and peripheral clock control. PLL configuration and
 * the system-clock initialization function live here; bus-frequency queries
 * are also portable. Any target-specific extensions live in the port header
 * (@c port/cortex-m4/navhal_port_clock.h on the Cortex-M4 port), included at the bottom.
 */

#ifndef HAL_CLOCK_H
#define HAL_CLOCK_H

/**
 * @defgroup HAL_CLOCK Clock
 * @ingroup HAL_DRIVERS
 * @brief System clock configuration and queries.
 * @{
 */

#include "common/hal_status.h"
#include "utils/clock_types.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


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
/**
 * @brief Configure the system clock.
 *
 * @param cfg Port-defined clock configuration. On ports whose clock is driven
 *            by a PLL the PLL parameters live in @c cfg (see the port's
 *            @c clock_types.h); there is no separate PLL argument.
 * @return ::HAL_OK on success.
 */
hal_status_t hal_clock_init(const hal_clock_config_t *cfg);

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

#if NAVHAL_CONFIG_DRV_CLOCK
#include "navhal_port_clock.h"
#endif


/** @} */ /* end of group HAL_CLOCK */
#endif /* HAL_CLOCK_H */
