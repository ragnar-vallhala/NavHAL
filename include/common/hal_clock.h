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
 * @brief Configure the system clock.
 *
 * @note Must be called before using other peripheral clocks.
 *
 * @param cfg Port-defined clock configuration. On ports whose clock is driven
 *            by a PLL the PLL parameters live in @c cfg (see the port's
 *            @c clock_types.h); there is no separate PLL argument.
 * @return ::HAL_OK on success.
 */
hal_status_t hal_clock_init(const hal_clock_config_t *cfg);

/** @brief Get the system clock frequency (SYSCLK) in Hz. */
uint32_t hal_clock_get_sysclk(void);

/**
 * @brief Number of separately-clocked buses this port reports.
 *
 * Zero on a port with no bus hierarchy (AVR, x86), where every peripheral
 * runs from SYSCLK and ::hal_clock_get_sysclk is the only clock query needed.
 */
uint8_t hal_clock_get_bus_count(void);

/**
 * @brief Get a bus clock frequency in Hz.
 *
 * @param bus Index below ::hal_clock_get_bus_count. Ports name their buses in
 *            their own @c hal_clock_bus_t; there is no portable bus vocabulary
 *            because bus topology is not a portable concept.
 * @return Frequency in Hz, or 0 if @p bus is not a bus this port has.
 */
uint32_t hal_clock_get_bus_clock(uint8_t bus);

#ifdef __cplusplus
} /* extern "C" */
#endif

#if NAVHAL_CONFIG_DRV_CLOCK
#include "navhal_port_clock.h"
#endif


/** @} */ /* end of group HAL_CLOCK */
#endif /* HAL_CLOCK_H */
