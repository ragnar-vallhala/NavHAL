#ifndef CORTEX_M4_GPIO_H
#define CORTEX_M4_GPIO_H

/**
 * @file gpio.h
 * @brief HAL interface for GPIO control on STM32F4 series.
 * @details
 * This header declares high-level functions for configuring and using
 * General-Purpose I/O (GPIO) pins, including mode setup, digital read/write,
 * pull-up/pull-down configuration, RCC enabling, and alternate function selection.
 *
 * These functions provide an abstraction layer over the low-level
 * register access defined in `cortex_m4_gpio_reg.h`.
 */

/**
 * @file gpio.h
 * @brief HAL interface for GPIO control on STM32F4 series.
 * @details
 * This header declares high-level functions for configuring and using
 * General-Purpose I/O (GPIO) pins, including mode setup, digital read/write,
 * pull-up/pull-down configuration, RCC enabling, and alternate function selection.
 *
 * These functions provide an abstraction layer over the low-level
 * register access defined in `cortex_m4_gpio_reg.h`.
 */

#include "utils/gpio_types.h"
/**
 * @brief Configure the mode and pull-up/pull-down for a GPIO pin.
 * @param pin Pin identifier.
 * @param mode GPIO mode (input, output, alternate function, analog).
 * @param pupd Pull-up/pull-down configuration.
 * @code
 * // Example: Configure PA5 as output with no pull-up/pull-down
 * hal_gpio_setmode(PA5, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_NO_PULL);
 * @endcode
 */
void hal_gpio_setmode(hal_gpio_pin pin, hal_gpio_mode mode,
                      hal_gpio_pullup_pulldown pupd);

/**
 * @brief Get the current mode of a GPIO pin.
 * @param pin Pin identifier.
 * @return The current GPIO mode of the pin.
 */
hal_gpio_mode hal_gpio_getmode(hal_gpio_pin pin);

/**
 * @brief Write a digital value to a GPIO pin.
 * @param pin Pin identifier.
 * @param state Desired pin state (high or low).
 * @code
 * // Example: Set PA5 high
 * hal_gpio_digitalwrite(PA5, HAL_GPIO_HIGH);
 * @endcode
 */
void hal_gpio_digitalwrite(hal_gpio_pin pin, hal_gpio_state state);

/**
 * @brief Read the digital value from a GPIO pin.
 * @param pin Pin identifier.
 * @return Current pin state (high or low).
 */
hal_gpio_state hal_gpio_digitalread(hal_gpio_pin pin);

/**
 * @brief Enable the RCC clock for the GPIO port of a given pin.
 * @param pin Pin identifier.
 * @note Must be called before configuring or using the pin.
 */
void hal_gpio_enable_rcc(hal_gpio_pin pin);

/**
 * @brief Configure the alternate function of a GPIO pin.
 * @param pin Pin identifier.
 * @param alt_fn Alternate function number/type.
 * @code
 * // Example: Set PA2 to USART2_TX alternate function
 * hal_gpio_set_alternate_function(PA2, HAL_GPIO_AF_USART2);
 * @endcode
 */
void hal_gpio_set_alternate_function(hal_gpio_pin pin,
                                     hal_gpio_alternate_function_t alt_fn);

void hal_gpio_set_output_type(hal_gpio_pin pin, hal_gpio_output_type otyper);
void hal_gpio_set_output_speed(hal_gpio_pin pin, hal_gpio_output_speed speed);
/** @} */ // end of GPIO_API

#endif // CORTEX_M4_GPIO_H
