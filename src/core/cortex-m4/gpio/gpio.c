/**
 * @file gpio.c
 * @brief HAL GPIO driver implementation for STM32F4 series
 * @details
 * This file contains the implementation of GPIO hardware abstraction layer
 * functions for STM32F4 microcontrollers. It provides complete GPIO pin
 * configuration and control capabilities including:
 * - Pin mode configuration (input/output/alternate/analog)
 * - Digital input/output operations
 * - Alternate function selection
 * - Output speed and type configuration
 * - Built-in pull-up/pull-down resistor control
 *
 * All functions handle the required RCC clock enabling automatically.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "core/cortex-m4/gpio.h"
#include "core/cortex-m4/gpio_reg.h"
#include "core/cortex-m4/rcc_reg.h"

/**
 * @brief Configure the mode and pull-up/pull-down for a GPIO pin
 * @param pin Pin identifier (e.g., PA5, PC13)
 * @param mode Desired pin mode (input/output/alternate/analog)
 * @param pupd Pull-up/pull-down configuration
 *
 * @details
 * This function:
 * 1. Enables the GPIO port clock via RCC
 * 2. Configures the pin mode in MODER register
 * 3. Sets pull-up/pull-down resistors in PUPDR register
 *
 * @note Automatically enables the GPIO port clock if not already enabled
 * @code
 * // Example: Configure PA5 as output with no pull-up/pull-down
 * hal_gpio_setmode(PA5, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_NO_PULL);
 * @endcode
 */
void hal_gpio_setmode(hal_gpio_pin pin, hal_gpio_mode mode,
                      hal_gpio_pullup_pulldown pupd) {
  hal_gpio_enable_rcc(pin); // Ensure GPIO port clock enabled

  // Set the mode bits for the pin
  GPIO_GET_PORT(pin)->MODER &=
      ~(0x3 << (GPIO_GET_PIN(pin) * 2)); // clear mode bits for moder
  GPIO_GET_PORT(pin)->MODER |= ((mode & 0x3) << (GPIO_GET_PIN(pin) * 2));

  // Configure pull-up/pull-down resistor
  GPIO_GET_PORT(pin)->PUPDR &=
      ~(0x3 << (GPIO_GET_PIN(pin) * 2)); // clear mode bits for pupdr
  GPIO_GET_PORT(pin)->PUPDR |= ((uint8_t)pupd & 0x3)
                               << (GPIO_GET_PIN(pin) * 2); // Set new pupd bits
}

/**
 * @brief Get the current mode of a GPIO pin
 * @param pin Pin identifier to query
 * @return Current GPIO mode of the specified pin
 *
 * @details
 * Reads the MODER register for the specified pin and returns
 * the current mode configuration.
 */
hal_gpio_mode hal_gpio_getmode(hal_gpio_pin pin) {
  return ((GPIO_GET_PORT(pin)->MODER) >> (GPIO_GET_PIN(pin) * 2)) & 0x3;
}

/**
 * @brief Write a digital value to a GPIO pin
 * @param pin Pin identifier to control
 * @param state Desired output state (HIGH or LOW)
 *
 * @details
 * Uses the BSRR register for atomic set/reset operations to
 * avoid read-modify-write sequences.
 *
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
 * @brief Read the digital value from a GPIO pin
 * @param pin Pin identifier to read
 * @return Current digital state of the pin (HIGH or LOW)
 *
 * @details
 * Reads the IDR register for the specified pin and returns
 * the current input state.
 */
hal_gpio_state hal_gpio_digitalread(hal_gpio_pin pin) {
  return (GPIO_GET_PORT(pin)->IDR >> GPIO_GET_PIN(pin)) & 0x1;
}

/**
 * @brief Enable the RCC clock for a GPIO port
 * @param pin Pin identifier whose port clock should be enabled
 *
 * @details
 * Enables the AHB1 peripheral clock for the GPIO port containing
 * the specified pin. This is automatically called by other GPIO
 * functions when needed.
 */
void hal_gpio_enable_rcc(hal_gpio_pin pin) {
  if (!(RCC->AHB1ENR & (1 << GPIO_GET_PORT_NUMBER(pin))))
    RCC->AHB1ENR |= (1 << GPIO_GET_PORT_NUMBER(pin));
}

/**
 * @brief Configure alternate function for a GPIO pin
 * @param pin Pin identifier to configure
 * @param alt_fn Alternate function number/type to assign
 *
 * @details
 * Configures the specified pin for alternate function mode and
 * sets the appropriate alternate function selection in AFRL/AFRH.
 * Automatically enables the GPIO port clock if needed.
 *
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

/**
 * @brief Configure GPIO output type
 * @param pin Pin identifier to configure
 * @param otyper Output type (push-pull or open-drain)
 *
 * @details
 * Sets the output type in the OTYPER register for the specified pin.
 * Does not change the pin mode - must be configured separately.
 */
void hal_gpio_set_output_type(hal_gpio_pin pin, hal_gpio_output_type otyper) {
  GPIO_GET_PORT(pin)->OTYPER &= ~(0x1 << GPIO_GET_PIN(pin));
  GPIO_GET_PORT(pin)->OTYPER |= ((otyper & 0x1) << GPIO_GET_PIN(pin));
}

/**
 * @brief Configure GPIO output speed
 * @param pin Pin identifier to configure
 * @param speed Desired output speed (low/medium/high/very high)
 *
 * @details
 * Sets the output speed in the OSPEEDR register for the specified pin.
 * Does not change the pin mode - must be configured separately.
 */
void hal_gpio_set_output_speed(hal_gpio_pin pin, hal_gpio_output_speed speed) {
  GPIO_GET_PORT(pin)->OSPEEDR &= ~(0x3 << GPIO_GET_PIN(pin));
  GPIO_GET_PORT(pin)->OSPEEDR |= ((speed & 0x3) << GPIO_GET_PIN(pin));
}