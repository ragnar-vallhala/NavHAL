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
 * @file hal_dwt.h
 * @brief Portable HAL interface for the cycle counter.
 *
 * @details
 * Backed by the Cortex-M4 DWT unit on the current port; the public API is
 * named @c hal_cycle_counter_* so it stays architecture-neutral (a target
 * without DWT can provide an equivalent counter, or gate the feature off via
 * @c NAVHAL_HAS_CYCLE_COUNTER).
 */

#ifndef HAL_DWT_H
#define HAL_DWT_H

#include "common/hal_status.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize and start the cycle counter.
 * @return ::HAL_OK.
 */
hal_status_t hal_cycle_counter_init(void);

/**
 * @brief Get the current cycle count.
 * @return 32-bit cycle count.
 */
uint32_t hal_cycle_counter_get(void);

/**
 * @brief Reset the cycle counter to zero.
 * @return ::HAL_OK.
 */
hal_status_t hal_cycle_counter_reset(void);

/**
 * @brief Busy-wait for a number of processor cycles.
 * @param cycles Number of cycles to delay.
 */
void hal_cycle_counter_delay(uint32_t cycles);

/**
 * @brief Cycles per microsecond at the current sysclk.
 *
 * @details Cached at ::hal_cycle_counter_init time from
 * ::hal_clock_get_sysclk — call @c hal_clock_init first. Returns 0 if
 * ::hal_cycle_counter_init has not run yet.
 */
uint32_t hal_cycle_counter_cycles_per_us(void);

/**
 * @brief Microseconds since the last counter reset.
 *
 * @details Derived from the 32-bit cycle counter. Wraps at
 * `UINT32_MAX / cycles_per_us` microseconds (≈51 s at 84 MHz). Use
 * unsigned subtraction for delta math.
 */
uint32_t hal_cycle_counter_get_us(void);

/**
 * @brief Busy-wait for the given number of microseconds.
 *
 * @details Internally `us * cycles_per_us` must fit in a 32-bit value;
 * at 84 MHz the largest single-call wait is ≈51 s. Split longer waits
 * across multiple calls.
 */
void hal_cycle_counter_delay_us(uint32_t us);

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Port-specific bits (compat shim, deprecated names). */
#include "navhal_port_dwt.h"

#endif /* HAL_DWT_H */
