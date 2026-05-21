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

#include "common/navhal_compiler.h"


#ifdef __cplusplus
extern "C" {
#endif
/**
 * @enum hal_gpio_pin
 * @brief Enumeration of GPIO pins across ports A to E.
 *
 * The naming convention is GPIO_PxYY where x is the port letter and
 * YY is the pin number on that port.
 */
typedef enum {
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

/*
 * Each enum below carries its standardized HAL_GPIO_* constants plus the
 * pre-standardization GPIO_* aliases. The aliases are marked deprecated, so
 * using one emits a compiler warning pointing at the standardized name.
 * All deprecated aliases are retained as a backward-compat surface behind NAVHAL_DEPRECATED.
 */

/**
 * @enum hal_gpio_mode_t
 * @brief GPIO pin mode (values match the STM32 MODER field encoding).
 */
typedef enum {
  HAL_GPIO_MODE_INPUT = 0,  /**< Digital input. */
  HAL_GPIO_MODE_OUTPUT = 1, /**< Digital output. */
  HAL_GPIO_MODE_AF = 2,     /**< Alternate function. */
  HAL_GPIO_MODE_ANALOG = 3, /**< Analog. */
  GPIO_INPUT NAVHAL_DEPRECATED("use HAL_GPIO_MODE_INPUT") = 0,
  GPIO_OUTPUT NAVHAL_DEPRECATED("use HAL_GPIO_MODE_OUTPUT") = 1,
  GPIO_AF NAVHAL_DEPRECATED("use HAL_GPIO_MODE_AF") = 2,
  GPIO_ANALOG NAVHAL_DEPRECATED("use HAL_GPIO_MODE_ANALOG") = 3,
} hal_gpio_mode_t;

/**
 * @enum hal_gpio_state_t
 * @brief GPIO pin logic level.
 */
typedef enum {
  HAL_GPIO_LOW = 0,  /**< Logic low. */
  HAL_GPIO_HIGH = 1, /**< Logic high. */
  GPIO_LOW NAVHAL_DEPRECATED("use HAL_GPIO_LOW") = 0,
  GPIO_HIGH NAVHAL_DEPRECATED("use HAL_GPIO_HIGH") = 1,
} hal_gpio_state_t;

/**
 * @enum hal_gpio_pull_t
 * @brief GPIO pull-up/pull-down resistor configuration.
 */
typedef enum {
  HAL_GPIO_PULL_NONE = 0, /**< No pull resistor. */
  HAL_GPIO_PULL_UP = 1,   /**< Pull-up resistor enabled. */
  HAL_GPIO_PULL_DOWN = 2, /**< Pull-down resistor enabled. */
  GPIO_PUPD_NONE NAVHAL_DEPRECATED("use HAL_GPIO_PULL_NONE") = 0,
  GPIO_PULLUP NAVHAL_DEPRECATED("use HAL_GPIO_PULL_UP") = 1,
  GPIO_PULLDOWN NAVHAL_DEPRECATED("use HAL_GPIO_PULL_DOWN") = 2,
} hal_gpio_pull_t;

/**
 * @enum hal_gpio_output_type_t
 * @brief GPIO output driver type.
 */
typedef enum {
  HAL_GPIO_OTYPE_PUSH_PULL = 0,  /**< Push-pull output. */
  HAL_GPIO_OTYPE_OPEN_DRAIN = 1, /**< Open-drain output. */
  GPIO_PUSH_PULL NAVHAL_DEPRECATED("use HAL_GPIO_OTYPE_PUSH_PULL") = 0,
  GPIO_OPEN_DRAIN NAVHAL_DEPRECATED("use HAL_GPIO_OTYPE_OPEN_DRAIN") = 1,
} hal_gpio_output_type_t;

/**
 * @enum hal_gpio_output_speed_t
 * @brief GPIO output slew-rate / speed.
 */
typedef enum {
  HAL_GPIO_SPEED_LOW = 0,       /**< Low speed. */
  HAL_GPIO_SPEED_MEDIUM = 1,    /**< Medium speed. */
  HAL_GPIO_SPEED_HIGH = 2,      /**< High speed. */
  HAL_GPIO_SPEED_VERY_HIGH = 3, /**< Very high speed. */
  GPIO_LOW_SPEED NAVHAL_DEPRECATED("use HAL_GPIO_SPEED_LOW") = 0,
  GPIO_MEDIUM_SPEED NAVHAL_DEPRECATED("use HAL_GPIO_SPEED_MEDIUM") = 1,
  GPIO_HIGH_SPEED NAVHAL_DEPRECATED("use HAL_GPIO_SPEED_HIGH") = 2,
  GPIO_VERY_HIGH_SPEED NAVHAL_DEPRECATED("use HAL_GPIO_SPEED_VERY_HIGH") = 3,
} hal_gpio_output_speed_t;

/**
 * @enum hal_gpio_af_t
 * @brief GPIO alternate-function selector (AF0..AF15).
 */
typedef enum {
  HAL_GPIO_AF0 = 0,
  HAL_GPIO_AF1,
  HAL_GPIO_AF2,
  HAL_GPIO_AF3,
  HAL_GPIO_AF4,
  HAL_GPIO_AF5,
  HAL_GPIO_AF6,
  HAL_GPIO_AF7,
  HAL_GPIO_AF8,
  HAL_GPIO_AF9,
  HAL_GPIO_AF10,
  HAL_GPIO_AF11,
  HAL_GPIO_AF12,
  HAL_GPIO_AF13,
  HAL_GPIO_AF14,
  HAL_GPIO_AF15,
  GPIO_AF00 NAVHAL_DEPRECATED("use HAL_GPIO_AF0") = 0,
  GPIO_AF01 NAVHAL_DEPRECATED("use HAL_GPIO_AF1") = 1,
  GPIO_AF02 NAVHAL_DEPRECATED("use HAL_GPIO_AF2") = 2,
  GPIO_AF03 NAVHAL_DEPRECATED("use HAL_GPIO_AF3") = 3,
  GPIO_AF04 NAVHAL_DEPRECATED("use HAL_GPIO_AF4") = 4,
  GPIO_AF05 NAVHAL_DEPRECATED("use HAL_GPIO_AF5") = 5,
  GPIO_AF06 NAVHAL_DEPRECATED("use HAL_GPIO_AF6") = 6,
  GPIO_AF07 NAVHAL_DEPRECATED("use HAL_GPIO_AF7") = 7,
  GPIO_AF08 NAVHAL_DEPRECATED("use HAL_GPIO_AF8") = 8,
  GPIO_AF09 NAVHAL_DEPRECATED("use HAL_GPIO_AF9") = 9,
  GPIO_AF10 NAVHAL_DEPRECATED("use HAL_GPIO_AF10") = 10,
  GPIO_AF11 NAVHAL_DEPRECATED("use HAL_GPIO_AF11") = 11,
  GPIO_AF12 NAVHAL_DEPRECATED("use HAL_GPIO_AF12") = 12,
  GPIO_AF13 NAVHAL_DEPRECATED("use HAL_GPIO_AF13") = 13,
  GPIO_AF14 NAVHAL_DEPRECATED("use HAL_GPIO_AF14") = 14,
  GPIO_AF15 NAVHAL_DEPRECATED("use HAL_GPIO_AF15") = 15,
} hal_gpio_af_t;

/* -------------------------------------------------------------------------- *
 * Deprecated — pre-standardization GPIO type names. Retained as a backward-compat alias.
 * -------------------------------------------------------------------------- */
typedef hal_gpio_mode_t hal_gpio_mode
    NAVHAL_DEPRECATED("use hal_gpio_mode_t");
typedef hal_gpio_state_t hal_gpio_state
    NAVHAL_DEPRECATED("use hal_gpio_state_t");
typedef hal_gpio_pull_t hal_gpio_pullup_pulldown
    NAVHAL_DEPRECATED("use hal_gpio_pull_t");
typedef hal_gpio_output_type_t hal_gpio_output_type
    NAVHAL_DEPRECATED("use hal_gpio_output_type_t");
typedef hal_gpio_output_speed_t hal_gpio_output_speed
    NAVHAL_DEPRECATED("use hal_gpio_output_speed_t");
typedef hal_gpio_af_t hal_gpio_alternate_function_t
    NAVHAL_DEPRECATED("use hal_gpio_af_t");

/**
 * @brief Standardized type name for a GPIO pin identifier.
 *
 * NavHAL GPIO pins use a two-layer model (see Section 7 of
 * `docs/api_standardization.md`):
 *
 * - **Core layer (per-MCU):** the @ref hal_gpio_pin enumeration above names
 *   each physical pin (`GPIO_PA05`, ...). The constant set is target-defined;
 *   STM32F401RE enumerates ports A-E plus H. Drivers and the HAL contract
 *   operate on this layer.
 * - **Board layer (per-board):** each board's `board.h` adds macro aliases
 *   (`D5`, `LED_BUILTIN`, ...) that resolve to core enum constants. Portable
 *   application code uses these.
 *
 * `hal_gpio_pin_t` is the canonical, standards-compliant spelling; the bare
 * `hal_gpio_pin` enum tag is retained for source compatibility.
 */
typedef hal_gpio_pin hal_gpio_pin_t;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // GPIO_TYPES_H
