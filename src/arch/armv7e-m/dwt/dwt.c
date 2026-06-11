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
 * @file dwt.c
 * @brief Standardized HAL cycle-counter driver for Cortex-M4 (DWT-backed).
 *
 * @details
 * Implements the standardized `hal_cycle_counter_*` API declared in
 * `port/cortex-m4/navhal_port_dwt.h`, using the Cortex-M4 DWT unit for high-resolution
 * cycle counting and timing.
 */

#include "navhal_port_config.h"
#ifdef _DWT_ENABLED

#include "navhal_port_clock.h"
#include "navhal_port_dwt.h"
#include "family/dwt_reg.h"

// Cached at init time from hal_clock_get_sysclk(). Stale if sysclk is
// reconfigured after init — call hal_cycle_counter_init again to refresh.
static uint32_t _cycles_per_us = 0;

hal_status_t hal_cycle_counter_init(void) {
  // 1. Enable CoreDebug TRCENA
  CoreDebug->DEMCR |= CORE_DEBUG_DEMCR_TRCENA_BIT;

  // 2. Clear cycle counter
  DWT->CYCCNT = 0;

  // 3. Enable CYCCNTENA
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_BIT;

  // 4. Cache cycles-per-microsecond. hal_clock_init must run before this.
  uint32_t hz = hal_clock_get_sysclk();
  _cycles_per_us = hz / 1000000U;
  if (_cycles_per_us == 0) {
    _cycles_per_us = 1; // sysclk < 1 MHz: clamp so delay_us still ticks
  }
  return HAL_OK;
}

uint32_t hal_cycle_counter_get(void) { return DWT->CYCCNT; }

hal_status_t hal_cycle_counter_reset(void) {
  DWT->CYCCNT = 0;
  return HAL_OK;
}

void hal_cycle_counter_delay(uint32_t cycles) {
  uint32_t start = hal_cycle_counter_get();
  while ((hal_cycle_counter_get() - start) < cycles) {
    __asm__ volatile("nop");
  }
}

uint32_t hal_cycle_counter_cycles_per_us(void) { return _cycles_per_us; }

uint32_t hal_cycle_counter_get_us(void) {
  return _cycles_per_us ? (DWT->CYCCNT / _cycles_per_us) : 0;
}

void hal_cycle_counter_delay_us(uint32_t us) {
  hal_cycle_counter_delay(us * _cycles_per_us);
}

#endif /* _DWT_ENABLED */
