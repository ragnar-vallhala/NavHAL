#include "core/cortex-m4/gpio.h"
#include "core/cortex-m4/gpio_reg.h"
#include "core/cortex-m4/rcc_reg.h"

/**
 * @file gpio.c
 * @brief HAL GPIO driver implementation for STM32F4 series.
 * @details
 * This file contains the implementation of the high-level HAL functions
 * declared in `gpio.h` for configuring and controlling GPIO pins.
 *
 * The functions in this file handle:
 * - Pin mode configuration
 * - Pull-up/pull-down settings
 * - Digital read/write operations
 * - RCC clock enabling for GPIO ports
 * - Alternate function selection
 */

/**
 * @brief Configure the mode and pull-up/pull-down for a GPIO pin.
 * @param pin Pin identifier.
 * @param mode GPIO mode (input, output, alternate function, analog).
 * @param pupd Pull-up/pull-down configuration.
 * @note This function automatically enables the RCC clock for the GPIO port.
 * @code
 * // Example: Configure PA5 as output with no pull-up/pull-down
 * hal_gpio_setmode(PA5, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_NO_PULL);
 * @endcode
 */
void hal_gpio_setmode(hal_gpio_pin pin, hal_gpio_mode mode,
                      hal_gpio_pullup_pulldown pupd) {
  hal_gpio_enable_rcc(pin); // Ensure GPIO port clock enabled

  // Set the mode bits for the pin
  GPIO_GET_PORT(pin)->MODER |= ((mode & 0x3) << (GPIO_GET_PIN(pin) * 2));

  // Configure pull-up/pull-down resistor
  GPIO_GET_PORT(pin)->PUPDR |= ((uint8_t)pupd & 0x3)
                               << (GPIO_GET_PIN(pin) * 2); // Set new pupd bits
}

/**
 * @brief Get the current mode of a GPIO pin.
 * @param pin Pin identifier.
 * @return The current GPIO mode of the pin.
 */
hal_gpio_mode hal_gpio_getmode(hal_gpio_pin pin) {
  return ((GPIO_GET_PORT(pin)->MODER) >> (GPIO_GET_PIN(pin) * 2)) & 0x3;
}

/**
 * @brief Write a digital value to a GPIO pin.
 * @param pin Pin identifier.
 * @param state Desired pin state (high or low).
 * @code
 * // Example: Set PA5 high
 * hal_gpio_digitalwrite(PA5, HAL_GPIO_HIGH);
 * @endcode
 */
void hal_gpio_digitalwrite(hal_gpio_pin pin, hal_gpio_state state) {
  if (state)
    GPIO_GET_PORT(pin)->BSRR |= (1 << GPIO_GET_PIN(pin)); // Set pin
  else
    GPIO_GET_PORT(pin)->BSRR |= (1 << (GPIO_GET_PIN(pin) + 16)); // Reset pin
}

/**
 * @brief Read the digital value from a GPIO pin.
 * @param pin Pin identifier.
 * @return Current pin state (high or low).
 */
hal_gpio_state hal_gpio_digitalread(hal_gpio_pin pin) {
  return (GPIO_GET_PORT(pin)->IDR >> GPIO_GET_PIN(pin)) & 0x1;
}

/**
 * @brief Enable the RCC clock for the GPIO port of a given pin.
 * @param pin Pin identifier.
 * @note This function is usually called automatically by other GPIO functions,
 * but can be called manually if required.
 */
void hal_gpio_enable_rcc(hal_gpio_pin pin) {
  if (!(RCC->AHB1ENR & (1 << GPIO_GET_PORT_NUMBER(pin))))
    RCC->AHB1ENR |= (1 << GPIO_GET_PORT_NUMBER(pin));
}

/**
 * @brief Configure the alternate function of a GPIO pin.
 * @param pin Pin identifier.
 * @param alt_fn Alternate function number/type.
 * @note This function will internally set the pin mode to alternate function mode.
 * @code
 * // Example: Set PA2 to USART2_TX alternate function
 * hal_gpio_set_alternate_function(PA2, HAL_GPIO_AF_USART2);
 * @endcode
 */
void hal_gpio_set_alternate_function(hal_gpio_pin pin,
                                     hal_gpio_alternate_function_t alt_fn) {
  hal_gpio_setmode(pin, GPIO_AF, GPIO_PUPD_NONE);
  uint8_t pin_num = GPIO_GET_PIN(pin);
  uint32_t mask = 0xF << (4 * (pin_num % 8));
  if (pin_num < 8) {
    GPIO_GET_PORT(pin)->AFRL &= ~mask;                     // clear bits
    GPIO_GET_PORT(pin)->AFRL |= (alt_fn << (4 * pin_num)); // set new value
  } else {
    GPIO_GET_PORT(pin)->AFRH &= ~mask; // clear bits
    GPIO_GET_PORT(pin)->AFRH |=
        (alt_fn << (4 * (pin_num % 8))); // set new value
  }
}

void hal_gpio_set_output_type(hal_gpio_pin pin, hal_gpio_output_type otyper) {
  GPIO_GET_PORT(pin)->OTYPER &= ~(0x1 << GPIO_GET_PIN(pin));
  GPIO_GET_PORT(pin)->OTYPER |= ((otyper & 0x1) << GPIO_GET_PIN(pin));
}
void hal_gpio_set_output_speed(hal_gpio_pin pin, hal_gpio_output_speed speed) {
  GPIO_GET_PORT(pin)->OSPEEDR &= ~(0x3 << GPIO_GET_PIN(pin));
  GPIO_GET_PORT(pin)->OSPEEDR |= ((speed & 0x3) << GPIO_GET_PIN(pin));
}
