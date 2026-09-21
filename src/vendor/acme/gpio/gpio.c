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
 * @file vendor/acme/gpio/gpio.c
 * @brief ACME1 GPIO backend -- the whole of this port's GPIO contribution.
 *
 * @details
 * Seven static functions and one ops table. No public @c hal_gpio_* symbol is
 * defined here: the shared layer in @c src/common/hal_gpio.c owns the API and
 * the argument checks, and dispatches through ::_hal_gpio_ops.
 */

#include "common/hal_gpio.h"
#include "internal/hal_gpio_ops.h"

#include "family/gpio_reg.h"

#include <stddef.h>

/** Two-bit field helper: clear then set the field for @p pin. */
static inline void _set_field2(volatile uint32_t *reg, uint8_t pin,
                               uint32_t value) {
  uint32_t shift = (uint32_t)pin * 2u;
  *reg = (*reg & ~(0x3u << shift)) | ((value & 0x3u) << shift);
}

static hal_status_t acme_gpio_enable_clock(hal_gpio_pin_t pin) {
  ACME_GPIO_CLKEN |= (1u << GPIO_GET_PORT_NUMBER(pin));
  return HAL_OK;
}

static hal_status_t acme_gpio_set_mode(hal_gpio_pin_t pin, hal_gpio_mode_t mode,
                                       hal_gpio_pull_t pull) {
  GPIO_Reg_Typedef *port = GPIO_GET_PORT(pin);
  uint8_t n = GPIO_GET_PIN(pin);

  acme_gpio_enable_clock(pin);
  _set_field2(&port->MODER, n, (uint32_t)mode);
  _set_field2(&port->PUPDR, n, (uint32_t)pull);
  return HAL_OK;
}

static hal_gpio_mode_t acme_gpio_get_mode(hal_gpio_pin_t pin) {
  GPIO_Reg_Typedef *port = GPIO_GET_PORT(pin);
  uint32_t shift = (uint32_t)GPIO_GET_PIN(pin) * 2u;
  return (hal_gpio_mode_t)((port->MODER >> shift) & 0x3u);
}

static hal_status_t acme_gpio_set_output_type(hal_gpio_pin_t pin,
                                              hal_gpio_output_type_t type) {
  GPIO_Reg_Typedef *port = GPIO_GET_PORT(pin);
  uint8_t n = GPIO_GET_PIN(pin);

  if (type == HAL_GPIO_OTYPE_OPEN_DRAIN)
    port->OTYPER |= (1u << n);
  else
    port->OTYPER &= ~(1u << n);
  return HAL_OK;
}

static hal_status_t acme_gpio_set_output_speed(hal_gpio_pin_t pin,
                                               hal_gpio_output_speed_t speed) {
  _set_field2(&GPIO_GET_PORT(pin)->OSPEEDR, GPIO_GET_PIN(pin),
              (uint32_t)speed);
  return HAL_OK;
}

static hal_status_t acme_gpio_set_alternate_function(hal_gpio_pin_t pin,
                                                     hal_gpio_af_t af) {
  GPIO_Reg_Typedef *port = GPIO_GET_PORT(pin);
  uint8_t n = GPIO_GET_PIN(pin);

  acme_gpio_enable_clock(pin);
  /* Switch to AF mode without touching PUPDR, so a pull configured alongside
   * the alternate function survives. */
  _set_field2(&port->MODER, n, (uint32_t)HAL_GPIO_MODE_AF);

  uint32_t idx = (n < 8u) ? 0u : 1u;
  uint32_t shift = ((uint32_t)n % 8u) * 4u;
  port->AFR[idx] = (port->AFR[idx] & ~(0xFu << shift)) |
                   (((uint32_t)af & 0xFu) << shift);
  return HAL_OK;
}

static hal_status_t acme_gpio_init(hal_gpio_pin_t pin,
                                   const hal_gpio_config_t *cfg) {
  /* cfg is non-NULL: the shared layer validated it before dispatching. */
  acme_gpio_set_mode(pin, cfg->mode, cfg->pull);
  acme_gpio_set_output_type(pin, cfg->output_type);
  acme_gpio_set_output_speed(pin, cfg->output_speed);
  return HAL_OK;
}

/** @brief The ACME1 GPIO backend. */
const hal_gpio_ops_t _hal_gpio_ops = {
    .init = acme_gpio_init,
    .set_mode = acme_gpio_set_mode,
    .get_mode = acme_gpio_get_mode,
    .enable_clock = acme_gpio_enable_clock,
    .set_alternate_function = acme_gpio_set_alternate_function,
    .set_output_type = acme_gpio_set_output_type,
    .set_output_speed = acme_gpio_set_output_speed,
};
