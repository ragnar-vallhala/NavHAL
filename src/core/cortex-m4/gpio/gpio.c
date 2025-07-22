/**
 * @file gpio.c
 * @brief Cortex-M4 (STM32F4) GPIO HAL Implementation.
 *
 * This file provides the implementation of the GPIO Hardware Abstraction Layer (HAL)
 * for Cortex-M4 based STM32F4 microcontrollers, specifically the STM32F401RE.
 * It includes functions for configuring GPIO pins, reading and writing digital states,
 * and managing GPIO port clocks through the RCC.
 *
 * Key features:
 * - Configure GPIO pin modes (input, output, alternate function, analog)
 * - Configure pull-up and pull-down resistors
 * - Atomic digital write using BSRR register
 * - Read digital input pin states
 * - Automatic enabling of RCC clocks for GPIO ports
 *
 * The GPIO pins are encoded as port and pin numbers, supporting ports A–E and H.
 * The code handles port indexing and provides safety checks on invalid ports.
 *
 * @author Ashutosh Vishwakarma
 * @date 2025-07-20
 */

#include "core/cortex-m4/gpio.h"

/**
 * @brief Extract GPIO port index from pin number.
 *
 * The pin number encodes port and pin within port.
 * Ports 6 and 5 are invalid, port 7 maps to index 5 (GPIOH).
 *
 * @param n Encoded GPIO pin number.
 * @return Port index in GPIO_BASE array.
 */
uint8_t _get_port(uint16_t n)
{
    uint8_t port = n / 16;
    if (port < 0 || port > 7 || port == 6 || port == 5)
        return 0; // Wrong port numbers, default to port 0 (GPIOA)
    if (port == 7)
        return 5; // Correct index in the GPIO_BASE array (GPIOH)
    return port;
}

/**
 * @brief Extract pin number within port from pin number.
 *
 * @param n Encoded GPIO pin number.
 * @return Pin number within port (0–15).
 */
uint8_t _get_pin(uint16_t n)
{
    return n % 16;
}

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
void hal_gpio_setmode(hal_gpio_pin pin, hal_gpio_mode mode, hal_gpio_pullup_pulldown pupd)
{
    uint8_t port_number = _get_port(pin);
    uint8_t pin_number = _get_pin(pin);
    hal_gpio_enable_rcc(pin); // Ensure GPIO port clock enabled

    volatile uint32_t *moder = GPIO_BASE[port_number] + (GPIO_MODER_OFFSET / sizeof(uint32_t));

    // Clear the 2 bits corresponding to the pin mode
    *moder &= ~(0x3 << (pin_number * 2));

    // Set the mode bits for the pin
    *moder |= ((mode & 0x3) << (pin_number * 2));

    // Configure pull-up/pull-down resistor
    volatile uint32_t *pupdr = GPIO_BASE[port_number] + (GPIO_PUPDR_OFFSET / sizeof(uint32_t));
    *pupdr &= ~(0x3 << (pin_number * 2));                // Clear existing pupd bits
    *pupdr |= ((uint8_t)pupd & 0x3) << (pin_number * 2); // Set new pupd bits
}

/**
 * @brief Get the mode configuration of a GPIO pin.
 *
 * Reads the mode bits from the MODER register.
 *
 * @param pin GPIO pin to query.
 * @return Current mode setting of the pin.
 */
hal_gpio_mode hal_gpio_getmode(hal_gpio_pin pin)
{
    uint8_t port_number = _get_port(pin);
    uint8_t pin_number = _get_pin(pin);
    volatile uint32_t *moder = GPIO_BASE[port_number] + (GPIO_MODER_OFFSET / sizeof(uint32_t));
    return ((*moder) >> (pin_number * 2)) & 0x3;
}

/**
 * @brief Write a digital logic level to a GPIO pin.
 *
 * Uses atomic BSRR register to set/reset pin atomically.
 *
 * @param pin GPIO pin to write to.
 * @param state Logic level to set (HIGH or LOW).
 */
void hal_gpio_digitalwrite(hal_gpio_pin pin, hal_gpio_state state)
{
    uint8_t port_number = _get_port(pin);
    uint8_t pin_number = _get_pin(pin);

    volatile uint32_t *bsrr = GPIO_BASE[port_number] + (GPIO_BSRR_OFFSET / 4);

    if (state)
        *bsrr |= (1 << pin_number); // Set pin
    else
        *bsrr |= (1 << (pin_number + 16)); // Reset pin
}

/**
 * @brief Read the current digital logic level of a GPIO pin.
 *
 * Reads from the input data register (IDR).
 *
 * @param pin GPIO pin to read.
 * @return Current logic level (HIGH or LOW).
 */
hal_gpio_state hal_gpio_digitalread(hal_gpio_pin pin)
{
    uint8_t port_number = _get_port(pin);
    uint8_t pin_number = _get_pin(pin);

    volatile uint32_t *idr = GPIO_BASE[port_number] + (GPIO_IDR_OFFSET / 4);
    return (*idr >> pin_number) & 0x1;
}

/**
 * @brief Enable the RCC clock for the GPIO port.
 *
 * Enables the clock for the GPIO port in the RCC AHB1 enable register.
 * Does nothing if already enabled.
 *
 * @param pin GPIO pin whose port clock to enable.
 */
void hal_gpio_enable_rcc(hal_gpio_pin pin)
{
    uint8_t port_number = _get_port(pin);

    if (!(RCC_AHB1ENR & (1 << port_number)))
        RCC_AHB1ENR |= (1 << port_number);
}

void hal_gpio_set_alternate_function(hal_gpio_pin pin, hal_gpio_alternate_function_t alt_fn)
{
    uint8_t port_number = _get_port(pin);
    uint8_t pin_number = _get_pin(pin);
    hal_gpio_setmode(pin, GPIO_AF, GPIO_PUPD_NONE);
    uint8_t offset = pin_number > 7 ? GPIO_AFRH_OFFSET : GPIO_AFRL_OFFSET;
    volatile uint32_t *afr = GPIO_BASE[port_number] + (offset / sizeof(uint32_t));
    *afr &= (0xF << (4 * (pin_number % 8)));
    *afr |= (alt_fn << (4 * (pin_number % 8)));
}