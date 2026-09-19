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
 * @file internal/hal_watchdog_ops.h
 * @brief HAL-internal independent-watchdog vendor backend interface (the driver vtable).
 *
 * @details
 * Not part of the public API -- application code includes
 * @c common/hal_watchdog.h. Each port defines exactly one ::_hal_watchdog_ops with
 * its register work; the shared layer in @c src/common/hal_watchdog.c validates
 * arguments once and dispatches here.
 *
 * @c get_timeout_ms and @c is_running stay backend entries rather than
 * state cached by the shared layer: the STM32 IWDG keeps running across a
 * system reset, so after a watchdog reset the hardware is armed while any
 * shared-layer flag would read false. Only the hardware knows.
 *
 * The window watchdog is a separate driver (@c DRV_WWDG, absent on AVR) with
 * its own table, rather than optional entries here that a port would leave
 * NULL.
 */

#ifndef NAVHAL_INTERNAL_HAL_WATCHDOG_OPS_H
#define NAVHAL_INTERNAL_HAL_WATCHDOG_OPS_H

#include "common/hal_watchdog.h"

#include <stdbool.h>
#include "common/hal_status.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Per-port watchdog operations table. */
typedef struct {
  /** Backend for ::hal_watchdog_start. */
  hal_status_t (*start)(uint32_t timeout_ms);
  /** Backend for ::hal_watchdog_kick. */
  hal_status_t (*kick)(void);
  /** Backend for ::hal_watchdog_get_timeout_ms. */
  uint32_t (*get_timeout_ms)(void);
  /** Backend for ::hal_watchdog_is_running. */
  bool (*is_running)(void);
  /** Backend for ::hal_watchdog_max_timeout_ms. */
  uint32_t (*max_timeout_ms)(void);
} hal_watchdog_ops_t;

/** @brief The active port's watchdog backend (defined by one backend). */
extern const hal_watchdog_ops_t _hal_watchdog_ops;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_INTERNAL_HAL_WATCHDOG_OPS_H */
