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
 * @file gpio.c
 * @brief STM32F4 (Cortex-M4) GPIO vendor backend.
 *
 * @details
 * Provides the STM32F4 implementations behind the GPIO driver vtable
 * (::hal_gpio_ops_t, declared in @c internal/hal_gpio_ops.h). These are the
 * register-poking operations only — argument validation (e.g. the NULL-config
 * check) lives once in the shared public layer @c src/common/hal_gpio.c and
 * has already run by the time these are called. The active table is published
 * as ::_hal_gpio_ops at the bottom of this file.
 *
 * The hot-path write/read/toggle helpers stay @c static @c inline in
 * @c port/cortex-m4/navhal_port_gpio.h.
 */

#include "internal/hal_gpio_ops.h"
#include "family/gpio_reg.h"
#include "family/rcc_reg.h"

static hal_status_t stm32_gpio_enable_clock(hal_gpio_pin_t pin) {
  uint32_t port = GPIO_GET_PORT_NUMBER(pin);
  if (!(RCC->AHB1ENR & (1U << port)))
    RCC->AHB1ENR |= (1U << port);
  return HAL_OK;
}

static hal_status_t stm32_gpio_set_mode(hal_gpio_pin_t pin, hal_gpio_mode_t mode,
                                        hal_gpio_pull_t pull) {
  stm32_gpio_enable_clock(pin); // Ensure GPIO port clock enabled

  uint32_t shift = GPIO_GET_PIN(pin) * 2;

  // Set the mode bits for the pin
  GPIO_GET_PORT(pin)->MODER &= ~(0x3U << shift);
  GPIO_GET_PORT(pin)->MODER |= (((uint32_t)mode & 0x3U) << shift);

  // Configure pull-up/pull-down resistor
  GPIO_GET_PORT(pin)->PUPDR &= ~(0x3U << shift);
  GPIO_GET_PORT(pin)->PUPDR |= (((uint32_t)pull & 0x3U) << shift);

  return HAL_OK;
}

static hal_gpio_mode_t stm32_gpio_get_mode(hal_gpio_pin_t pin) {
  return (hal_gpio_mode_t)((GPIO_GET_PORT(pin)->MODER >>
                            (GPIO_GET_PIN(pin) * 2)) &
                           0x3U);
}

static hal_status_t stm32_gpio_set_alternate_function(hal_gpio_pin_t pin,
                                                      hal_gpio_af_t af) {
  stm32_gpio_set_mode(pin, HAL_GPIO_MODE_AF, HAL_GPIO_PULL_NONE);

  uint8_t pin_num = GPIO_GET_PIN(pin);
  uint32_t mask = 0xFU << (4 * (pin_num % 8));
  if (pin_num < 8) {
    GPIO_GET_PORT(pin)->AFRL &= ~mask;
    GPIO_GET_PORT(pin)->AFRL |= ((uint32_t)af << (4 * pin_num));
  } else {
    GPIO_GET_PORT(pin)->AFRH &= ~mask;
    GPIO_GET_PORT(pin)->AFRH |= ((uint32_t)af << (4 * (pin_num % 8)));
  }
  return HAL_OK;
}

static hal_status_t stm32_gpio_set_output_type(hal_gpio_pin_t pin,
                                               hal_gpio_output_type_t type) {
  GPIO_GET_PORT(pin)->OTYPER &= ~(0x1U << GPIO_GET_PIN(pin));
  GPIO_GET_PORT(pin)->OTYPER |= (((uint32_t)type & 0x1U) << GPIO_GET_PIN(pin));
  return HAL_OK;
}

static hal_status_t stm32_gpio_set_output_speed(hal_gpio_pin_t pin,
                                                hal_gpio_output_speed_t speed) {
  /* OSPEEDR is 2 bits per pin (M4 RM0383 §8.4.3); the shift must scale. */
  uint32_t shift = GPIO_GET_PIN(pin) * 2;
  GPIO_GET_PORT(pin)->OSPEEDR &= ~(0x3U << shift);
  GPIO_GET_PORT(pin)->OSPEEDR |= (((uint32_t)speed & 0x3U) << shift);
  return HAL_OK;
}

static hal_status_t stm32_gpio_init(hal_gpio_pin_t pin,
                                    const hal_gpio_config_t *cfg) {
  /* cfg is non-NULL: the public layer validated it before dispatching. */
  stm32_gpio_set_mode(pin, cfg->mode, cfg->pull);

  if (cfg->mode == HAL_GPIO_MODE_OUTPUT || cfg->mode == HAL_GPIO_MODE_AF) {
    stm32_gpio_set_output_type(pin, cfg->output_type);
    stm32_gpio_set_output_speed(pin, cfg->output_speed);
  }
  if (cfg->mode == HAL_GPIO_MODE_AF)
    stm32_gpio_set_alternate_function(pin, cfg->alternate);

  return HAL_OK;
}

const hal_gpio_ops_t _hal_gpio_ops = {
    .init = stm32_gpio_init,
    .set_mode = stm32_gpio_set_mode,
    .get_mode = stm32_gpio_get_mode,
    .enable_clock = stm32_gpio_enable_clock,
    .set_alternate_function = stm32_gpio_set_alternate_function,
    .set_output_type = stm32_gpio_set_output_type,
    .set_output_speed = stm32_gpio_set_output_speed,
};
