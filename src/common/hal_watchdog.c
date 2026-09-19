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
 * @file common/hal_watchdog.c
 * @brief Shared public watchdog layer: validate, then dispatch to the vendor vtable.
 *
 * @details
 * The one implementation of the public @c hal_watchdog_* API. Argument checks
 * live here so every port agrees on them, and the backends keep only the
 * register work.
 */

#include "common/hal_watchdog.h"
#include "internal/hal_watchdog_ops.h"

hal_status_t hal_watchdog_start(uint32_t timeout_ms) {
  if (timeout_ms == 0u)
    return HAL_ERR_INVALID_ARG;
  return _hal_watchdog_ops.start(timeout_ms);
}

hal_status_t hal_watchdog_kick(void) { return _hal_watchdog_ops.kick(); }

uint32_t hal_watchdog_get_timeout_ms(void) {
  return _hal_watchdog_ops.get_timeout_ms();
}

bool hal_watchdog_is_running(void) { return _hal_watchdog_ops.is_running(); }

uint32_t hal_watchdog_max_timeout_ms(void) {
  return _hal_watchdog_ops.max_timeout_ms();
}
