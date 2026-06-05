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
 * @file src/common/hal_gpio.c
 * @brief Shared public GPIO layer: validate, then dispatch to the vendor vtable.
 *
 * @details
 * This is the one implementation of the public @c hal_gpio_* configuration API
 * that every port shares. It performs the argument validation that used to be
 * copy-pasted into each vendor's @c gpio.c (today: the NULL-config check on
 * ::hal_gpio_init) and then forwards to the active port's ::_hal_gpio_ops
 * backend. Vendor drivers keep only the register-poking implementations.
 *
 * The hot-path accessors (write / read / toggle) are not routed through here —
 * they remain @c static @c inline in the port header.
 */

#include "common/hal_gpio.h"
#include "internal/hal_gpio_ops.h"

#include <stddef.h>

hal_status_t hal_gpio_init(hal_gpio_pin_t pin, const hal_gpio_config_t *cfg) {
  if (cfg == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_gpio_ops.init(pin, cfg);
}

hal_status_t hal_gpio_set_mode(hal_gpio_pin_t pin, hal_gpio_mode_t mode,
                               hal_gpio_pull_t pull) {
  return _hal_gpio_ops.set_mode(pin, mode, pull);
}

hal_gpio_mode_t hal_gpio_get_mode(hal_gpio_pin_t pin) {
  return _hal_gpio_ops.get_mode(pin);
}

hal_status_t hal_gpio_enable_clock(hal_gpio_pin_t pin) {
  return _hal_gpio_ops.enable_clock(pin);
}

hal_status_t hal_gpio_set_alternate_function(hal_gpio_pin_t pin,
                                             hal_gpio_af_t af) {
  return _hal_gpio_ops.set_alternate_function(pin, af);
}

hal_status_t hal_gpio_set_output_type(hal_gpio_pin_t pin,
                                      hal_gpio_output_type_t type) {
  return _hal_gpio_ops.set_output_type(pin, type);
}

hal_status_t hal_gpio_set_output_speed(hal_gpio_pin_t pin,
                                       hal_gpio_output_speed_t speed) {
  return _hal_gpio_ops.set_output_speed(pin, speed);
}
