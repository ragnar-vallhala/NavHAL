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
 * @file internal/hal_gpio_ops.h
 * @brief HAL-internal GPIO vendor-backend interface (the driver vtable).
 *
 * @details
 * This header is **not part of the public API** — application code includes
 * @c common/hal_gpio.h, never this file. It declares the per-vendor operations
 * table that the shared public layer (@c src/common/hal_gpio.c) dispatches
 * through. Each port supplies exactly one definition of ::_hal_gpio_ops with
 * its register-poking implementations; adding a vendor means filling in this
 * table, not re-declaring the public API.
 *
 * The table is embedded **directly** (a @c const object, not a
 * pointer-to-table): this drops one level of indirection on every dispatch,
 * and because the object is @c const and resolved at link time, @c -flto
 * devirtualises the indirect call back to a direct call (and inlines it) on a
 * release build. See @ref roadmap_abstraction for the measured per-arch costs.
 *
 * Hot-path accessors (write / read / toggle) are deliberately **not** in this
 * table — they stay @c static @c inline in the port header so a constant pin
 * still folds to a single instruction.
 */

#ifndef NAVHAL_INTERNAL_HAL_GPIO_OPS_H
#define NAVHAL_INTERNAL_HAL_GPIO_OPS_H

#include "common/hal_gpio.h"
#include "common/hal_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Per-vendor GPIO operations table.
 *
 * One function pointer per public @c hal_gpio_* configuration call. The
 * shared public layer validates arguments once, then dispatches here. A port
 * that leaves an entry NULL is caught at build time
 * (@c -Wmissing-field-initializers) or, failing that, at the call site.
 */
typedef struct {
  /** Backend for ::hal_gpio_init. Validation (NULL @p cfg) has already run. */
  hal_status_t (*init)(hal_gpio_pin_t pin, const hal_gpio_config_t *cfg);
  /** Backend for ::hal_gpio_set_mode. */
  hal_status_t (*set_mode)(hal_gpio_pin_t pin, hal_gpio_mode_t mode,
                           hal_gpio_pull_t pull);
  /** Backend for ::hal_gpio_get_mode. */
  hal_gpio_mode_t (*get_mode)(hal_gpio_pin_t pin);
  /** Backend for ::hal_gpio_enable_clock. */
  hal_status_t (*enable_clock)(hal_gpio_pin_t pin);
  /** Backend for ::hal_gpio_set_alternate_function. */
  hal_status_t (*set_alternate_function)(hal_gpio_pin_t pin, hal_gpio_af_t af);
  /** Backend for ::hal_gpio_set_output_type. */
  hal_status_t (*set_output_type)(hal_gpio_pin_t pin,
                                  hal_gpio_output_type_t type);
  /** Backend for ::hal_gpio_set_output_speed. */
  hal_status_t (*set_output_speed)(hal_gpio_pin_t pin,
                                   hal_gpio_output_speed_t speed);
} hal_gpio_ops_t;

/**
 * @brief The active port's GPIO operations table.
 *
 * Defined by exactly one vendor translation unit (e.g.
 * @c src/vendor/stm32/gpio/gpio.c). NavHAL builds one vendor per firmware, so
 * the symbol is resolved uniquely at link time.
 */
extern const hal_gpio_ops_t _hal_gpio_ops;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_INTERNAL_HAL_GPIO_OPS_H */
