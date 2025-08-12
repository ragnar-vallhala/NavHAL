/**
 * @file gpio.h
 * @brief GPIO HAL implementation for Cortex-M4 (STM32F401RE).
 *
 * This header provides low-level register definitions and functions to
 * configure and control GPIO peripherals on Cortex-M4-based microcontrollers,
 * specifically STM32F401RE in this implementation.
 *
 * @ingroup HAL_GPIO
 *
 * @note This file is architecture-specific. It is included through the common
 * `hal_gpio.h` dispatcher based on the target definition (e.g., `CORTEX_M4`).
 *
 * @author Ashutosh Vishwakarma
 * @date 2025-07-20
 */

#ifndef CORTEX_M4_GPIO_H
#define CORTEX_M4_GPIO_H

#include "utils/gpio_types.h"
/**
 * @brief Set the mode and pull configuration of a GPIO pin.
 *
 * @param pin   The GPIO pin (PORTx_PINy encoded).
 * @param mode  The pin mode (input, output, alternate, analog).
 * @param pupd  Pull configuration (no pull, pull-up, pull-down).
 */
void hal_gpio_setmode(hal_gpio_pin pin, hal_gpio_mode mode,
                      hal_gpio_pullup_pulldown pupd);

/**
 * @brief Get the current mode of a GPIO pin.
 *
 * @param pin The GPIO pin.
 * @return The current mode of the pin.
 */
hal_gpio_mode hal_gpio_getmode(hal_gpio_pin pin);

/**
 * @brief Write a logic level to a GPIO pin.
 *
 * @param pin   The GPIO pin to write to.
 * @param state GPIO_HIGH or GPIO_LOW.
 */
void hal_gpio_digitalwrite(hal_gpio_pin pin, hal_gpio_state state);

/**
 * @brief Read the logic level from a GPIO pin.
 *
 * @param pin The GPIO pin to read from.
 * @return GPIO_HIGH or GPIO_LOW.
 */
hal_gpio_state hal_gpio_digitalread(hal_gpio_pin pin);

/**
 * @brief Enable the RCC peripheral clock for the GPIO port.
 *
 * @param pin GPIO pin whose port needs to be enabled.
 */
void hal_gpio_enable_rcc(hal_gpio_pin pin);

/**
 * @brief Configure the alternate function for a GPIO pin.
 *
 * This function sets the alternate function for a given GPIO pin.
 * Alternate functions are used when the pin is to be controlled by
 * a peripheral (like UART, SPI, I2C, TIM, etc.) instead of used as
 * a regular input/output pin.
 *
 * @param pin     The GPIO pin to configure.
 * @param alt_fn  The alternate function number to assign (0–15 depending on MCU
 * support).
 */
void hal_gpio_set_alternate_function(hal_gpio_pin pin,
                                     hal_gpio_alternate_function_t alt_fn);

void hal_gpio_set_output_type(hal_gpio_pin pin, hal_gpio_output_type otyper);
void hal_gpio_set_output_speed(hal_gpio_pin pin, hal_gpio_output_speed speed);
/** @} */ // end of GPIO_API

#endif // CORTEX_M4_GPIO_H
