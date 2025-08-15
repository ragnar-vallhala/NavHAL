/**
 * @file core/cortex-m4/gpio.h
 * @brief Cortex-M4 specific GPIO HAL interface.
 *
 * @details
 * This header defines the function prototypes for configuring and controlling
 * GPIO pins on Cortex-M4 microcontrollers. Functions include setting pin modes,
 * reading and writing digital states, enabling clocks, and configuring alternate
 * functions, output type, and speed.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_GPIO_H
#define CORTEX_M4_GPIO_H

#include "utils/gpio_types.h"

/**
 * @brief Set the mode of a GPIO pin.
 *
 * @param pin The GPIO pin to configure.
 * @param mode The desired mode (input, output, alternate function, analog).
 * @param pupd Pull-up/pull-down configuration for the pin.
 */
void hal_gpio_setmode(hal_gpio_pin pin, hal_gpio_mode mode,
                      hal_gpio_pullup_pulldown pupd);

/**
 * @brief Get the current mode of a GPIO pin.
 *
 * @param pin The GPIO pin to query.
 * @return The current mode of the pin.
 */
hal_gpio_mode hal_gpio_getmode(hal_gpio_pin pin);

/**
 * @brief Write a digital state to a GPIO pin.
 *
 * @param pin The GPIO pin to write.
 * @param state The digital state to set (HIGH/LOW).
 */
void hal_gpio_digitalwrite(hal_gpio_pin pin, hal_gpio_state state);

/**
 * @brief Read the digital state from a GPIO pin.
 *
 * @param pin The GPIO pin to read.
 * @return The current digital state of the pin (HIGH/LOW).
 */
hal_gpio_state hal_gpio_digitalread(hal_gpio_pin pin);

/**
 * @brief Enable the RCC clock for a specific GPIO pin's port.
 *
 * @param pin The GPIO pin whose port clock is to be enabled.
 */
void hal_gpio_enable_rcc(hal_gpio_pin pin);

/**
 * @brief Set the alternate function for a GPIO pin.
 *
 * @param pin The GPIO pin to configure.
 * @param alt_fn The alternate function to assign to the pin.
 */
void hal_gpio_set_alternate_function(hal_gpio_pin pin,
                                     hal_gpio_alternate_function_t alt_fn);

/**
 * @brief Set the output type for a GPIO pin.
 *
 * @param pin The GPIO pin to configure.
 * @param otyper The output type (push-pull or open-drain).
 */
void hal_gpio_set_output_type(hal_gpio_pin pin, hal_gpio_output_type otyper);

/**
 * @brief Set the output speed for a GPIO pin.
 *
 * @param pin The GPIO pin to configure.
 * @param speed The output speed (low, medium, high, very high).
 */
void hal_gpio_set_output_speed(hal_gpio_pin pin, hal_gpio_output_speed speed);

#endif // CORTEX_M4_GPIO_H
