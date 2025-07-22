/**
 * @file gpio.h
 * @brief GPIO HAL implementation for Cortex-M4 (STM32F401RE).
 *
 * This header provides low-level register definitions and functions to configure
 * and control GPIO peripherals on Cortex-M4-based microcontrollers, specifically
 * STM32F401RE in this implementation.
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

#include "utils/types.h"

/** @defgroup GPIO_REGISTERS GPIO Register Map (STM32F401RE)
 *  @brief Memory-mapped I/O register definitions for GPIO.
 *  @{
 */

/**
 * @brief RCC AHB1 peripheral clock enable register.
 * @ref stm32f401re_reference.pdf - page 118
 */
#define RCC_AHB1ENR (*(volatile uint32_t *)0x40023830)

/**
 * @brief Base addresses of GPIO ports.
 * @ref stm32f401re.pdf - page 52
 */
#define GPIOA 0x40020000
#define GPIOB 0x40020400
#define GPIOC 0x40020800
#define GPIOD 0x40020C00
#define GPIOE 0x40021000
#define GPIOH 0x40021C00

/// Total number of GPIO ports supported (A–E and H)
#define GPIO_PORT_COUNT 6

/// Base address table for each GPIO port
static volatile uint32_t *const GPIO_BASE[GPIO_PORT_COUNT] = {
    (uint32_t *)GPIOA,
    (uint32_t *)GPIOB,
    (uint32_t *)GPIOC,
    (uint32_t *)GPIOD,
    (uint32_t *)GPIOE,
    (uint32_t *)GPIOH};

/** @} */ // end of GPIO_REGISTERS

/** @defgroup GPIO_REGISTER_OFFSETS GPIO Register Offsets
 *  @brief Offsets for specific control and data registers.
 *  @{
 */

/**
 * @brief GPIO mode register offset.
 * @ref stm32f401re_reference.pdf - page 158
 */
#define GPIO_MODER_OFFSET 0x00

#define GPIO_AFRL_OFFSET 0x20

#define GPIO_AFRH_OFFSET 0x24

/**
 * @brief GPIO output type register offset.
 * @ref stm32f401re_reference.pdf - page 158
 */
#define GPIO_OTYPER_OFFSET 0x04

/**
 * @brief GPIO pull-up/pull-down register offset.
 * @ref stm32f401re_reference.pdf - page 159
 */
#define GPIO_PUPDR_OFFSET 0x0C

/**
 * @brief GPIO input data register offset.
 * @ref stm32f401re_reference.pdf - page 160
 */
#define GPIO_IDR_OFFSET 0x10

/**
 * @brief GPIO output data register offset.
 * @ref stm32f401re_reference.pdf - page 160
 */
#define GPIO_ODR_OFFSET 0x14

/**
 * @brief GPIO bit set/reset register offset.
 * @ref stm32f401re_reference.pdf - page 161
 */
#define GPIO_BSRR_OFFSET 0x18

/** @} */ // end of GPIO_REGISTER_OFFSETS

/** @defgroup GPIO_API GPIO HAL Functions (Cortex-M4)
 *  @brief Function declarations for GPIO HAL.
 *  @{
 */

/**
 * @brief Set the mode and pull configuration of a GPIO pin.
 *
 * @param pin   The GPIO pin (PORTx_PINy encoded).
 * @param mode  The pin mode (input, output, alternate, analog).
 * @param pupd  Pull configuration (no pull, pull-up, pull-down).
 */
void hal_gpio_setmode(hal_gpio_pin pin, hal_gpio_mode mode, hal_gpio_pullup_pulldown pupd);

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

void hal_gpio_set_alternate_function(hal_gpio_pin pin, hal_gpio_alternate_function_t alt_fn);

/** @} */ // end of GPIO_API

#endif // CORTEX_M4_GPIO_H
