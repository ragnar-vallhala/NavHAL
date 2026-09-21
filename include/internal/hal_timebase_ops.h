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
 * @file internal/hal_timebase_ops.h
 * @brief HAL-internal timebase backend interface.
 *
 * @details
 * Three backends: SysTick on ARMv7E-M, Timer0 on the ATmega328P, and the TSC
 * with an optional 8254 PIT tick on the PC.
 *
 * This table mirrors its API rather than inverting it, and that is the right
 * shape here. The obvious candidate to lift is
 * @c get_millis == @c get_micros / 1000, which holds on ARM and AVR -- but
 * the PC reads the TSC and scales by cycles-per-millisecond directly, so
 * lifting it would route that port through a coarser path for no gain. The
 * delays are the same story: the AVR uses avr-libc's calibrated @c _delay_ms
 * rather than looping a microsecond delay, which is more accurate than a
 * shared implementation would be.
 *
 * What does hoist is the @c tick_us != 0 check, which all three repeated.
 *
 * @c hal_timebase_tick stays out: it is the hook a periodic ISR calls, and
 * only the ports whose timebase is interrupt-driven (ARM, PC) have one.
 */

#ifndef NAVHAL_INTERNAL_HAL_TIMEBASE_OPS_H
#define NAVHAL_INTERNAL_HAL_TIMEBASE_OPS_H

#include "common/hal_status.h"
#include "common/hal_timer.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Per-port timebase operations table. */
typedef struct {
  /** Start the timebase. @p tick_us is non-zero; a port may still reject a
   *  period its hardware cannot represent. */
  hal_status_t (*init)(uint32_t tick_us);

  /** Ticks since init. */
  uint32_t (*get_tick)(void);
  /** Configured tick period. */
  uint32_t (*get_tick_duration_us)(void);
  /** Reload/compare value programmed into the counter, 0 where there is none. */
  uint32_t (*get_reload_value)(void);

  /** Microseconds since init. */
  uint32_t (*get_micros)(void);
  /** Milliseconds since init. */
  uint32_t (*get_millis)(void);

  /** Busy-wait. */
  void (*delay_us)(uint32_t us);
  /** Busy-wait. */
  void (*delay_ms)(uint32_t ms);

  /** Install the per-tick callback, or NULL to clear it. */
  hal_status_t (*set_callback)(hal_timebase_callback_t cb);
} hal_timebase_ops_t;

/** @brief The active port's timebase backend. */
extern const hal_timebase_ops_t _hal_timebase_ops;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_INTERNAL_HAL_TIMEBASE_OPS_H */
