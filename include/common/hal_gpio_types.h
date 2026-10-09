/**
 * @file hal_gpio_types.h
 * @brief GPIO types, split out so the ops table and the port header can have
 *        them without including the API.
 *
 * common/hal_gpio.h ends by including the port header, and the port header --
 * with the family inline header and the ops table behind it -- needs these
 * types. Reaching them through the API header made the include graph circular,
 * which clang-tidy reports (misc-header-include-cycle) at every call site that
 * opens one. Include guards made it work; they did not make it a graph anyone
 * could follow.
 *
 * Types live here, the API stays in common/hal_gpio.h, and nothing below the
 * API needs to include it.
 */
#ifndef HAL_GPIO_TYPES_H
#define HAL_GPIO_TYPES_H

#include "common/hal_status.h"
#include "utils/gpio_types.h" /* port-resolved pin/state enums */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Aggregate configuration for a single GPIO pin.
 *
 * Passed to ::hal_gpio_init. The @c output_type and @c output_speed fields
 * apply only when @ref mode is ::HAL_GPIO_MODE_OUTPUT or ::HAL_GPIO_MODE_AF;
 * the @c alternate field applies only when @ref mode is ::HAL_GPIO_MODE_AF.
 */
typedef struct {
  hal_gpio_mode_t mode;                 /**< Pin mode. */
  hal_gpio_pull_t pull;                 /**< Pull-up/pull-down configuration. */
  hal_gpio_output_type_t output_type;   /**< Output driver type. */
  hal_gpio_output_speed_t output_speed; /**< Output slew rate. */
  hal_gpio_af_t alternate;              /**< Alternate function selector. */
} hal_gpio_config_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* HAL_GPIO_TYPES_H */
