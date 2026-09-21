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
 * @file src/common/hal_clock.c
 * @brief Shared public clock layer: validate, then dispatch to the vendor vtable.
 *
 * @details
 * The one implementation of the public @c hal_clock_* API. It hoists the
 * NULL-config check that every backend duplicated; the remaining checks
 * (STM32's "PLL source needs pll_cfg", AVR's prescaler range) touch
 * port-specific config fields, so they stay in their backends and the shared
 * layer remains arch-agnostic.
 */

#include "common/hal_clock.h"
#include "internal/hal_clock_ops.h"

#include <stddef.h>

hal_status_t hal_clock_init(const hal_clock_config_t *cfg) {
  if (cfg == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_clock_ops.init(cfg);
}

uint32_t hal_clock_get_sysclk(void) { return _hal_clock_ops.get_sysclk(); }

uint8_t hal_clock_get_bus_count(void) { return _hal_clock_ops.get_bus_count(); }

/* Bounds-checked here so no backend has to repeat it. */
uint32_t hal_clock_get_bus_clock(uint8_t bus) {
  if (bus >= _hal_clock_ops.get_bus_count())
    return 0u;
  return _hal_clock_ops.get_bus_clock(bus);
}
