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
 * @file family/gpio_inline.h
 * @brief ACME1 inlined GPIO accessors.
 *
 * @details
 * The hot path, kept out of ::hal_gpio_ops_t so a constant pin folds to a
 * single store. ACME has dedicated SET/CLR/TGL registers, so all three are
 * one instruction and none is a read-modify-write -- which is the point:
 * a vendor writes these against the silicon it actually has.
 */

#ifndef NAVHAL_ACME1_GPIO_INLINE_H
#define NAVHAL_ACME1_GPIO_INLINE_H

#include "common/hal_gpio.h"
#include "family/gpio_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Drive @p pin. */
static inline void hal_gpio_write(hal_gpio_pin_t pin, hal_gpio_state_t state) {
  GPIO_Reg_Typedef *port = GPIO_GET_PORT(pin);
  uint32_t bit = 1u << GPIO_GET_PIN(pin);

  if (state == HAL_GPIO_LOW)
    port->CLR = bit;
  else
    port->SET = bit;
}

/** @brief Sample @p pin. */
static inline hal_gpio_state_t hal_gpio_read(hal_gpio_pin_t pin) {
  return (hal_gpio_state_t)((GPIO_GET_PORT(pin)->IDR >> GPIO_GET_PIN(pin)) &
                            0x1u);
}

/** @brief Invert @p pin, atomically. */
static inline void hal_gpio_toggle(hal_gpio_pin_t pin) {
  GPIO_GET_PORT(pin)->TGL = 1u << GPIO_GET_PIN(pin);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_ACME1_GPIO_INLINE_H */
