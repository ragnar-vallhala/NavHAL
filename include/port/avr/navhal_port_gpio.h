/**
 * @file port/avr/navhal_port_gpio.h
 * @brief AVR / ATmega328P GPIO port header.
 *
 * @details
 * The portable GPIO prototypes live in @c common/hal_gpio.h, which includes
 * this header. The hot-path pin accessors (write / read / toggle) are
 * declared here and implemented in the AVR GPIO driver — unlike the
 * Cortex-M4 port they are not header-inline, because decoding a
 * ::hal_gpio_pin_t to an ATmega328P PORT/PIN register is done in the driver.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef NAVHAL_PORT_GPIO_H
#define NAVHAL_PORT_GPIO_H

#include "common/hal_gpio.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Write a logic level to a pin (hot path).
 * @param pin   Pin to drive.
 * @param state Logic level to apply.
 */
void hal_gpio_write(hal_gpio_pin_t pin, hal_gpio_state_t state);

/**
 * @brief Read the logic level of a pin (hot path).
 * @param pin Pin to read.
 * @return The pin's current ::hal_gpio_state_t.
 */
hal_gpio_state_t hal_gpio_read(hal_gpio_pin_t pin);

/**
 * @brief Toggle a pin's output level (hot path).
 * @param pin Pin to toggle.
 */
void hal_gpio_toggle(hal_gpio_pin_t pin);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* NAVHAL_PORT_GPIO_H */
