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
 * @file common/hal_timebase.c
 * @brief Shared public timebase layer.
 *
 * @details
 * All three backends rejected a zero tick period, with three slightly
 * different spellings. That check lives here now; anything a port's hardware
 * additionally cannot represent stays with the port.
 */

#include "common/hal_timer.h"
#include "internal/hal_timebase_ops.h"

hal_status_t hal_timebase_init(uint32_t tick_us) {
  if (tick_us == 0u)
    return HAL_ERR_INVALID_ARG;
  return _hal_timebase_ops.init(tick_us);
}

uint32_t hal_timebase_get_tick(void) { return _hal_timebase_ops.get_tick(); }

uint32_t hal_timebase_get_tick_duration_us(void) {
  return _hal_timebase_ops.get_tick_duration_us();
}

uint32_t hal_timebase_get_reload_value(void) {
  return _hal_timebase_ops.get_reload_value();
}

uint32_t hal_timebase_get_micros(void) {
  return _hal_timebase_ops.get_micros();
}

uint32_t hal_timebase_get_millis(void) {
  return _hal_timebase_ops.get_millis();
}

void hal_delay_us(uint32_t us) { _hal_timebase_ops.delay_us(us); }

void hal_delay_ms(uint32_t ms) { _hal_timebase_ops.delay_ms(ms); }

hal_status_t hal_timebase_set_callback(hal_timebase_callback_t cb) {
  return _hal_timebase_ops.set_callback(cb);
}
