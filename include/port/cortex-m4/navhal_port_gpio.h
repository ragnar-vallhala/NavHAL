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
 * @file port/cortex-m4/navhal_port_gpio.h
 * @brief Cortex-M4 / STM32F4 GPIO port header.
 *
 * @details
 * Provides the hot-path inline accessors that touch BSRR/IDR/ODR directly,
 * plus the deprecated-name shim. The public prototypes live in
 * @c common/hal_gpio.h, which includes this header.
 */

#ifndef NAVHAL_PORT_GPIO_H
#define NAVHAL_PORT_GPIO_H

#include "common/hal_gpio.h"
#include "family/gpio_reg.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Write a logic level to a pin (hot path).
 * @param pin   Pin to drive.
 * @param state Logic level to apply.
 */
static inline void hal_gpio_write(hal_gpio_pin_t pin, hal_gpio_state_t state) {
  if (state)
    GPIO_GET_PORT(pin)->BSRR = (1U << GPIO_GET_PIN(pin));
  else
    GPIO_GET_PORT(pin)->BSRR = (1U << (GPIO_GET_PIN(pin) + 16));
}

/**
 * @brief Read the logic level of a pin (hot path).
 * @param pin Pin to read.
 * @return The pin's current ::hal_gpio_state_t.
 */
static inline hal_gpio_state_t hal_gpio_read(hal_gpio_pin_t pin) {
  return (hal_gpio_state_t)((GPIO_GET_PORT(pin)->IDR >> GPIO_GET_PIN(pin)) &
                            0x1);
}

/**
 * @brief Toggle a pin's output level (hot path).
 * @param pin Pin to toggle.
 */
static inline void hal_gpio_toggle(hal_gpio_pin_t pin) {
  GPIO_GET_PORT(pin)->ODR ^= (1U << GPIO_GET_PIN(pin));
}

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Deprecated pre-standardization GPIO names — retained as a backward-compat alias. */
#include "compat/gpio_compat.h"

#endif /* NAVHAL_PORT_GPIO_H */
