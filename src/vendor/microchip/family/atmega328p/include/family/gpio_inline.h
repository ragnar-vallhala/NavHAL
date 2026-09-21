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
 * @brief ATmega328P inlined GPIO accessors.
 *
 * @details
 * The hot path, kept out of ::hal_gpio_ops_t on purpose: a function-pointer
 * call cannot fold to a single instruction, and at @c -Og a dispatch is a
 * real indirect call. These stay @c static @c inline so a constant pin
 * compiles to one store.
 *
 * They live with the vendor's register map rather than in the arch header,
 * because GPIO is silicon, not CPU core: a Cortex-M4 has no GPIO block, and
 * two vendors on one arch have nothing in common here. The arch header
 * includes this file and defines none of it.
 */

#ifndef NAVHAL_ATMEGA328P_GPIO_INLINE_H
#define NAVHAL_ATMEGA328P_GPIO_INLINE_H

#include "common/hal_gpio.h"
#include <avr/io.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Drive @p pin. */
static inline void hal_gpio_write(hal_gpio_pin_t pin, hal_gpio_state_t state) {
  uint8_t bit = (uint8_t)(1u << ((uint8_t)pin & 7u));
  uint8_t idx = (uint8_t)pin >> 3;
  volatile uint8_t *port =
      (idx == 0u) ? &PORTB : (idx == 1u) ? &PORTC : &PORTD;
  if (state == HAL_GPIO_LOW)
    *port &= (uint8_t)~bit;
  else
    *port |= bit;
}

/** @brief Sample @p pin. */
static inline hal_gpio_state_t hal_gpio_read(hal_gpio_pin_t pin) {
  uint8_t bit = (uint8_t)(1u << ((uint8_t)pin & 7u));
  uint8_t idx = (uint8_t)pin >> 3;
  volatile uint8_t *in = (idx == 0u) ? &PINB : (idx == 1u) ? &PINC : &PIND;
  return (*in & bit) ? HAL_GPIO_HIGH : HAL_GPIO_LOW;
}

/** @brief Invert @p pin. Writing a 1 to PINx toggles PORTx on this part, so
 *  the toggle is a single store rather than a read-modify-write. */
static inline void hal_gpio_toggle(hal_gpio_pin_t pin) {
  uint8_t bit = (uint8_t)(1u << ((uint8_t)pin & 7u));
  uint8_t idx = (uint8_t)pin >> 3;
  volatile uint8_t *pinreg =
      (idx == 0u) ? &PINB : (idx == 1u) ? &PINC : &PIND;
  *pinreg = bit;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_ATMEGA328P_GPIO_INLINE_H */
