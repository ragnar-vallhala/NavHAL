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

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Port-specific bits (compat shim, deprecated names). */
#include "navhal_port_dwt.h"

#endif /* HAL_DWT_H */
