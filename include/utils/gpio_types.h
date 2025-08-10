/**
 * @file gpio_types.h
 * @brief GPIO pin definitions and related types for NavHAL.
 *
 * This header defines enumerations for GPIO pins, modes,
 * logic states, and pull-up/pull-down configurations.
 *
 * These types provide a hardware-independent abstraction
 * for GPIO configuration and usage within NavHAL.
 *
 * @author Ashutosh Vishwakarma
 * @date 2025-07-20
 */

#ifndef GPIO_TYPES_H
#define GPIO_TYPES_H

/**
 * @enum hal_gpio_pin
 * @brief Enumeration of GPIO pins across ports A to E.
 *
 * The naming convention is GPIO_PxYY where x is the port letter and
 * YY is the pin number on that port.
 */
typedef enum
{
    GPIO_PA00,
    GPIO_PA01,
    GPIO_PA02,
    GPIO_PA03,
    GPIO_PA04,
    GPIO_PA05,
    GPIO_PA06,
    GPIO_PA07,
    GPIO_PA08,
    GPIO_PA09,
    GPIO_PA10,
    GPIO_PA11,
    GPIO_PA12,
    GPIO_PA13,
    GPIO_PA14,
    GPIO_PA15,

    GPIO_PB00,
    GPIO_PB01,
    GPIO_PB02,
    GPIO_PB03,
    GPIO_PB04,
    GPIO_PB05,
    GPIO_PB06,
    GPIO_PB07,
    GPIO_PB08,
    GPIO_PB09,
    GPIO_PB10,
    GPIO_PB11,
    GPIO_PB12,
    GPIO_PB13,
    GPIO_PB14,
    GPIO_PB15,

    GPIO_PC00,
    GPIO_PC01,
    GPIO_PC02,
    GPIO_PC03,
    GPIO_PC04,
    GPIO_PC05,
    GPIO_PC06,
    GPIO_PC07,
    GPIO_PC08,
    GPIO_PC09,
    GPIO_PC10,
    GPIO_PC11,
    GPIO_PC12,
    GPIO_PC13,
    GPIO_PC14,
    GPIO_PC15,

    GPIO_PD00,
    GPIO_PD01,
    GPIO_PD02,
    GPIO_PD03,
    GPIO_PD04,
    GPIO_PD05,
    GPIO_PD06,
    GPIO_PD07,
    GPIO_PD08,
    GPIO_PD09,
    GPIO_PD10,
    GPIO_PD11,
    GPIO_PD12,
    GPIO_PD13,
    GPIO_PD14,
    GPIO_PD15,

    GPIO_PE00,
    GPIO_PE01,
    GPIO_PE02,
    GPIO_PE03,
    GPIO_PE04,
    GPIO_PE05,
    GPIO_PE06,
    GPIO_PE07,
    GPIO_PE08,
    GPIO_PE09,
    GPIO_PE10,
    GPIO_PE11,
    GPIO_PE12,
    GPIO_PE13,
    GPIO_PE14,
    GPIO_PE15,

    GPIO_PH00,
    GPIO_PH01,
    GPIO_PH02,
    GPIO_PH03,
    GPIO_PH04,
    GPIO_PH05,
    GPIO_PH06,
    GPIO_PH07,
    GPIO_PH08,
    GPIO_PH09,
    GPIO_PH10,
    GPIO_PH11,
    GPIO_PH12,
    GPIO_PH13,
    GPIO_PH14,
    GPIO_PH15


} hal_gpio_pin;

/**
 * @enum hal_gpio_mode
 * @brief GPIO pin modes.
 */
typedef enum
{
    GPIO_INPUT,  /**< Configure pin as input */
    GPIO_OUTPUT, /**< Configure pin as output */
    GPIO_AF,     /**< Configure pin as alternate function */
    GPIO_ANALOG  /**< Configure pin as analog */
} hal_gpio_mode;

/**
 * @enum hal_gpio_state
 * @brief GPIO pin logic levels.
 */
typedef enum
{
    GPIO_LOW = 0, /**< Logic low state */
    GPIO_HIGH = 1 /**< Logic high state */
} hal_gpio_state;

/**
 * @enum hal_gpio_pullup_pulldown
 * @brief GPIO pull-up/pull-down resistor configuration.
 */
typedef enum
{
    GPIO_PUPD_NONE, /**< No pull resistor */
    GPIO_PULLUP,    /**< Pull-up resistor enabled */
    GPIO_PULLDOWN   /**< Pull-down resistor enabled */
} hal_gpio_pullup_pulldown;

typedef enum
{
    GPIO_AF00,
    GPIO_AF01,
    GPIO_AF02,
    GPIO_AF03,
    GPIO_AF04,
    GPIO_AF05,
    GPIO_AF06,
    GPIO_AF07,
    GPIO_AF08,
    GPIO_AF09,
    GPIO_AF10,
    GPIO_AF11,
    GPIO_AF12,
    GPIO_AF13,
    GPIO_AF14,
    GPIO_AF15
} hal_gpio_alternate_function_t;

#endif // GPIO_TYPES_H
