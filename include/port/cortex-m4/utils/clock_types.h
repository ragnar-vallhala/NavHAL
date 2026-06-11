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

#include "family/rcc_reg.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
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


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // CLOCK_TYPES_H
