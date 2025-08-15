/**
 * @file gpio.c
 * @brief Cortex-M4 (STM32F4) GPIO HAL Implementation.
 *
 * This file provides the implementation of the GPIO Hardware Abstraction Layer
 * (HAL) for Cortex-M4 based STM32F4 microcontrollers, specifically the
 * STM32F401RE. It includes functions for configuring GPIO pins, reading and
 * writing digital states, and managing GPIO port clocks through the RCC.
 *
 * Key features:
 * - Configure GPIO pin modes (input, output, alternate function, analog)
 * - Configure pull-up and pull-down resistors
 * - Atomic digital write using BSRR register
 * - Read digital input pin states
 * - Automatic enabling of RCC clocks for GPIO ports
 *
 * The GPIO pins are encoded as port and pin numbers, supporting ports A–E and
 * H. The code handles port indexing and provides safety checks on invalid
 * ports.
 *
 * @author Ashutosh Vishwakarma
 * @date 2025-07-20
 */

#include "core/cortex-m4/gpio.h"
#include "core/cortex-m4/gpio_reg.h"
#include "core/cortex-m4/rcc_reg.h"

/**
 * @brief Set GPIO pin mode and pull-up/pull-down configuration.
 *
 * Enables GPIO port clock if not already enabled.
 * Configures mode bits and pull-up/pull-down bits in respective registers.
 *
 * @param pin GPIO pin to configure.
 * @param mode Mode to set (input, output, alt, analog).
 * @param pupd Pull-up/pull-down configuration.
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
 * @brief Get the mode configuration of a GPIO pin.
 *
 * Reads the mode bits from the MODER register.
 *
 * @param pin GPIO pin to query.
 * @return Current mode setting of the pin.
 */
hal_gpio_mode hal_gpio_getmode(hal_gpio_pin pin) {
  return ((GPIO_GET_PORT(pin)->MODER) >> (GPIO_GET_PIN(pin) * 2)) & 0x3;
}

/**
 * @brief Write a digital logic level to a GPIO pin.
 *
 * Uses atomic BSRR register to set/reset pin atomically.
 *
 * @param pin GPIO pin to write to.
 * @param state Logic level to set (HIGH or LOW).
 */
void hal_gpio_digitalwrite(hal_gpio_pin pin, hal_gpio_state state) {
  if (state)
    GPIO_GET_PORT(pin)->BSRR |= (1 << GPIO_GET_PIN(pin)); // Set pin
  else
    GPIO_GET_PORT(pin)->BSRR |= (1 << (GPIO_GET_PIN(pin) + 16)); // Reset pin
}

/**
 * @brief Read the current digital logic level of a GPIO pin.
 *
 * Reads from the input data register (IDR).
 *
 * @param pin GPIO pin to read.
 * @return Current logic level (HIGH or LOW).
 */
hal_gpio_state hal_gpio_digitalread(hal_gpio_pin pin) {
  return (GPIO_GET_PORT(pin)->IDR >> GPIO_GET_PIN(pin)) & 0x1;
}

/**
 * @brief Enable the RCC clock for the GPIO port.
 *
 * Enables the clock for the GPIO port in the RCC AHB1 enable register.
 * Does nothing if already enabled.
 *
 * @param pin GPIO pin whose port clock to enable.
 */
void hal_gpio_enable_rcc(hal_gpio_pin pin) {
  if (!(RCC->AHB1ENR & (1 << GPIO_GET_PORT_NUMBER(pin))))
    RCC->AHB1ENR |= (1 << GPIO_GET_PORT_NUMBER(pin));
}

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
