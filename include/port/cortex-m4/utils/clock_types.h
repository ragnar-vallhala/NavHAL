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

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/** @brief This port drives its system clock through a PLL. */
#define NAVHAL_HAS_CLOCK_PLL 1

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
 * @brief PLL configuration.
 *
 * SYSCLK = (input / @c pll_m) * @c pll_n / @c pll_p. Only meaningful when
 * ::hal_clock_config_t::source is ::HAL_CLOCK_SOURCE_PLL.
 */
typedef struct {
  hal_clock_source_t input_src; /**< Clock input source for the PLL. */
  uint8_t pll_m;                /**< Division factor for the PLL input. */
  uint16_t pll_n;               /**< Multiplication factor for the PLL VCO. */
  uint8_t pll_p;                /**< Division factor for the system clock. */
  uint8_t pll_q;                /**< Division factor for peripheral clocks. */
} hal_pll_config_t;

/**
 * @brief System clock configuration structure.
 *
 * Bus dividers are plain divide-by-N values (1, 2, 4, ... ), not register
 * encodings: the backend maps them onto RCC's field values, so configuring a
 * clock needs no register header. A divider of 0 is treated as 1.
 */
typedef struct {
  hal_clock_source_t source; ///< Selected clock source (HSI, HSE, or PLL)
  uint16_t hpre_div;         ///< AHB divider (1..512), divide-by-N
  uint16_t ppre1_div;        ///< APB1 divider (1..16), divide-by-N
  uint16_t ppre2_div;        ///< APB2 divider (1..16), divide-by-N
  hal_pll_config_t pll;      ///< Used when @c source is HAL_CLOCK_SOURCE_PLL
} hal_clock_config_t;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // CLOCK_TYPES_H
