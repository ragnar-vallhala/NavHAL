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
 * @file src/vendor/microchip/clock/clock.c
 * @brief ATmega328P clock HAL driver.
 *
 * @details
 * Implements @c common/hal_clock.h for the ATmega328P. The oscillator source
 * is fuse-selected and cannot change at runtime, so @c hal_clock_init only
 * applies the system clock prescaler (CLKPR / CLKPS). The ATmega328P has no
 * PLL and no AHB/APB bus hierarchy — every peripheral runs from the single
 * (possibly prescaled) system clock, so all bus-frequency queries return it.
 */

#include "internal/hal_clock_ops.h"

#include <avr/io.h>
#include <avr/power.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Active CLKPS divider exponent (0 = /1 ... 8 = /256). */
static uint8_t s_prescaler_log2 = 0;

static hal_status_t avr_clock_init(const hal_clock_config_t *cfg) {
  /* cfg is non-NULL: the public layer validated it before dispatching. */
  if (cfg->prescaler_log2 > 8) /* CLKPS encodes /1 .. /256. */
    return HAL_ERR_INVALID_ARG;

  s_prescaler_log2 = cfg->prescaler_log2;
  /* clock_div_t is the exponent: clock_div_1 == 0 ... clock_div_256 == 8. */
  clock_prescale_set((clock_div_t)cfg->prescaler_log2);
  return HAL_OK;
}

static uint32_t avr_clock_get_sysclk(void) {
  return (uint32_t)F_CPU >> s_prescaler_log2;
}




/* No bus hierarchy: every peripheral runs from the core clock, so there is
 * nothing for hal_clock_get_bus_clock to report. Callers use get_sysclk. */
static uint8_t avr_clock_get_bus_count(void) { return 0u; }

static uint32_t avr_clock_get_bus_clock(uint8_t bus) {
  (void)bus;
  return 0u;
}

const hal_clock_ops_t _hal_clock_ops = {
    .init = avr_clock_init,
    .get_sysclk = avr_clock_get_sysclk,
    .get_bus_count = avr_clock_get_bus_count,
    .get_bus_clock = avr_clock_get_bus_clock,
};
