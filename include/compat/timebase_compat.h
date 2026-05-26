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
 * @file timebase_compat.h
 * @brief Deprecated pre-standardization timebase (SysTick) API shim.
 *
 * @details
 * Provides the pre-standardization SysTick / delay / tick function names as
 * deprecated inline wrappers over the standardized `hal_timebase_*` /
 * `hal_delay_*` API. Using a legacy name produces a compiler warning naming
 * the standardized replacement. Included automatically by
 * `port/cortex-m4/navhal_port_timer.h` after the standardized declarations.
 *
 * Retained as a backward-compat alias behind NAVHAL_DEPRECATED. New code MUST use the standardized names directly.
 */

#ifndef NAVHAL_TIMEBASE_COMPAT_H
#define NAVHAL_TIMEBASE_COMPAT_H

#include "common/hal_status.h"
#include "common/navhal_compiler.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
/** @deprecated Use hal_timebase_init(). */
NAVHAL_DEPRECATED("use hal_timebase_init")
static inline hal_status_t systick_init(uint32_t tick_us) {
  return hal_timebase_init(tick_us);
}

/** @deprecated Use hal_delay_ms(). */
NAVHAL_DEPRECATED("use hal_delay_ms")
static inline void delay_ms(uint32_t ms) { hal_delay_ms(ms); }

/** @deprecated Use hal_delay_us(). */
NAVHAL_DEPRECATED("use hal_delay_us")
static inline void delay_us(uint64_t us) { hal_delay_us((uint32_t)us); }

/** @deprecated Use hal_timebase_get_tick(). */
NAVHAL_DEPRECATED("use hal_timebase_get_tick")
static inline uint32_t hal_get_tick(void) { return hal_timebase_get_tick(); }

/** @deprecated Use hal_timebase_get_tick_duration_us(). */
NAVHAL_DEPRECATED("use hal_timebase_get_tick_duration_us")
static inline uint32_t hal_get_tick_duration_us(void) {
  return hal_timebase_get_tick_duration_us();
}

/** @deprecated Use hal_timebase_get_reload_value(). */
NAVHAL_DEPRECATED("use hal_timebase_get_reload_value")
static inline uint32_t hal_get_tick_reload_value(void) {
  return hal_timebase_get_reload_value();
}

/** @deprecated Use hal_timebase_get_millis(). */
NAVHAL_DEPRECATED("use hal_timebase_get_millis")
static inline uint32_t hal_get_millis(void) {
  return hal_timebase_get_millis();
}

/** @deprecated Use hal_timebase_get_micros(). */
NAVHAL_DEPRECATED("use hal_timebase_get_micros")
static inline uint32_t hal_get_micros(void) {
  return hal_timebase_get_micros();
}

/** @deprecated Use hal_timebase_set_callback(). */
NAVHAL_DEPRECATED("use hal_timebase_set_callback")
static inline void hal_systick_set_callback(hal_timebase_callback_t cb) {
  hal_timebase_set_callback(cb);
}


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* NAVHAL_TIMEBASE_COMPAT_H */
