/**
 * @file port/avr/navhal_port_gpio.h
 * @brief AVR / ATmega328P GPIO port header.
 *
 * @details
 * The portable GPIO prototypes (init, set_mode, get_mode, …) live in
 * @c common/hal_gpio.h, which includes this header. The hot-path pin
 * accessors — write / read / toggle — are defined here as @c static
 * @c inline: for a compile-time-constant pin (the common case) the
 * pin-to-register decode constant-folds away and avr-gcc emits a single
 * `sbi` / `cbi` (or a one-byte store for the PINx-write toggle). This gives
 * the AVR port the same zero-cost GPIO the Cortex-M4 port has — a runtime
 * pin variable still works, it just costs the decode.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_PORT_GPIO_H
#define NAVHAL_PORT_GPIO_H

#include "common/hal_gpio.h"

#include <avr/io.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Write a logic level to a pin (hot path).
 *
 * Folds to a single `sbi` / `cbi` when @p pin and @p state are constant.
 */
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

/**
 * @brief Read the logic level of a pin (hot path).
 */
static inline hal_gpio_state_t hal_gpio_read(hal_gpio_pin_t pin) {
  uint8_t bit = (uint8_t)(1u << ((uint8_t)pin & 7u));
  uint8_t idx = (uint8_t)pin >> 3;
  volatile uint8_t *in = (idx == 0u) ? &PINB : (idx == 1u) ? &PINC : &PIND;
  return (*in & bit) ? HAL_GPIO_HIGH : HAL_GPIO_LOW;
}

/**
 * @brief Toggle a pin's output level (hot path).
 *
 * Writing a 1 to a PINx bit toggles the matching PORTx bit on the
 * ATmega328P; for a constant @p pin this is a single one-byte store.
 */
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
#endif /* NAVHAL_PORT_GPIO_H */
