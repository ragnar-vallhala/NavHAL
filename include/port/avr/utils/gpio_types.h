/**
 * @file gpio_types.h
 * @brief GPIO pin definitions and related types — AVR / ATmega328P port.
 *
 * @details
 * Core layer of the two-layer GPIO model (Section 7 of
 * `docs/api_standardization.md`). The ATmega328P has ports B, C and D.
 * Pin identifiers are encoded 8-per-port so a driver can decode
 * `port = pin >> 3` (0=B, 1=C, 2=D) and `bit = pin & 7`.
 *
 * The mode / pull / output-type / output-speed / alternate-function enums
 * are the portable HAL contract — they carry the same `HAL_GPIO_*` values
 * on every port. Some have no ATmega328P hardware meaning (slew rate,
 * alternate function, pull-down); the AVR GPIO driver ignores those.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef GPIO_TYPES_H
#define GPIO_TYPES_H


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum hal_gpio_pin
 * @brief ATmega328P GPIO pins (ports B, C, D), encoded 8 per port.
 */
typedef enum {
  GPIO_PB00 = 0,
  GPIO_PB01 = 1,
  GPIO_PB02 = 2,
  GPIO_PB03 = 3,
  GPIO_PB04 = 4,
  GPIO_PB05 = 5,
  GPIO_PB06 = 6,
  GPIO_PB07 = 7,

  GPIO_PC00 = 8,
  GPIO_PC01 = 9,
  GPIO_PC02 = 10,
  GPIO_PC03 = 11,
  GPIO_PC04 = 12,
  GPIO_PC05 = 13,
  GPIO_PC06 = 14, /**< Shared with /RESET; GPIO only when RSTDISBL is set. */

  GPIO_PD00 = 16,
  GPIO_PD01 = 17,
  GPIO_PD02 = 18,
  GPIO_PD03 = 19,
  GPIO_PD04 = 20,
  GPIO_PD05 = 21,
  GPIO_PD06 = 22,
  GPIO_PD07 = 23,
} hal_gpio_pin;

/** @brief GPIO pin mode. ATmega328P implements INPUT and OUTPUT. */
typedef enum {
  HAL_GPIO_MODE_INPUT = 0,  /**< Digital input. */
  HAL_GPIO_MODE_OUTPUT = 1, /**< Digital output. */
  HAL_GPIO_MODE_AF = 2,     /**< Alternate function (no-op on ATmega328P). */
  HAL_GPIO_MODE_ANALOG = 3, /**< Analog (no-op; ADC pins are input). */
} hal_gpio_mode_t;

/** @brief GPIO pin logic level. */
typedef enum {
  HAL_GPIO_LOW = 0,  /**< Logic low. */
  HAL_GPIO_HIGH = 1, /**< Logic high. */
} hal_gpio_state_t;

/** @brief GPIO pull-resistor configuration. ATmega328P has pull-up only. */
typedef enum {
  HAL_GPIO_PULL_NONE = 0, /**< No pull resistor. */
  HAL_GPIO_PULL_UP = 1,   /**< Pull-up enabled. */
  HAL_GPIO_PULL_DOWN = 2, /**< Pull-down — unsupported on ATmega328P. */
} hal_gpio_pull_t;

/** @brief GPIO output driver type (no hardware effect on ATmega328P). */
typedef enum {
  HAL_GPIO_OTYPE_PUSH_PULL = 0,
  HAL_GPIO_OTYPE_OPEN_DRAIN = 1,
} hal_gpio_output_type_t;

/** @brief GPIO output slew rate (no hardware effect on ATmega328P). */
typedef enum {
  HAL_GPIO_SPEED_LOW = 0,
  HAL_GPIO_SPEED_MEDIUM = 1,
  HAL_GPIO_SPEED_HIGH = 2,
  HAL_GPIO_SPEED_VERY_HIGH = 3,
} hal_gpio_output_speed_t;

/** @brief GPIO alternate-function selector (no hardware effect on AVR). */
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
} hal_gpio_af_t;

/** @brief Standardized type name for a GPIO pin identifier. */
typedef hal_gpio_pin hal_gpio_pin_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* GPIO_TYPES_H */
