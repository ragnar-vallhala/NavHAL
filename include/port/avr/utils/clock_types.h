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
 * @brief Clock HAL type definitions — AVR / ATmega328P port.
 *
 * @details
 * The ATmega328P clock source is selected by fuse bits, not at runtime;
 * `hal_clock_config_t::source` is therefore informational. The one runtime
 * knob is the system clock prescaler (CLKPR / CLKPS), expressed here as a
 * power-of-two divider exponent.
 */

#ifndef CLOCK_TYPES_H
#define CLOCK_TYPES_H

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/** @brief System clock source (fuse-selected on ATmega328P). */
typedef enum {
  HAL_CLOCK_SOURCE_INTERNAL_RC,   /**< Internal ~8 MHz RC oscillator. */
  HAL_CLOCK_SOURCE_EXTERNAL_XTAL, /**< External crystal / resonator. */
} hal_clock_source_t;

/**
 * @brief System clock configuration.
 *
 * @c source documents the fuse-selected oscillator; @c prescaler_log2 is the
 * runtime CLKPS divider exponent (0 = /1, 1 = /2, ... 8 = /256).
 */
typedef struct {
  hal_clock_source_t source; /**< Informational; actual source is fuse-set. */
  uint8_t prescaler_log2;    /**< CLKPS divider exponent, 0..8. */
} hal_clock_config_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* CLOCK_TYPES_H */
