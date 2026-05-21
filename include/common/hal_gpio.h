/**
 * @file hal_gpio.h
 * @brief Portable HAL interface for General Purpose Input/Output (GPIO).
 *
 * @details
 * Holds the public GPIO API surface: configuration aggregate, status-returning
 * function prototypes, and the typedef-only support pulled in from
 * @c utils/gpio_types.h. Hot-path accessors (write/read/toggle) and any
 * register-backed bits live in the target port header
 * (@c port/cortex-m4/navhal_port_gpio.h on the Cortex-M4 port), included at the bottom of
 * this file so callers only need to include @c common/hal_gpio.h.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_GPIO_H
#define HAL_GPIO_H

#include "common/hal_status.h"
#include "utils/gpio_types.h"


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

/**
 * @brief Configure a GPIO pin from an aggregate ::hal_gpio_config_t.
 * @param pin Pin to configure.
 * @param cfg Configuration; must not be NULL.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG if @p cfg is NULL.
 */
hal_status_t hal_gpio_init(hal_gpio_pin_t pin, const hal_gpio_config_t *cfg);

/**
 * @brief Set a pin's mode and pull configuration.
 * @param pin  Pin to configure.
 * @param mode Desired mode.
 * @param pull Pull-up/pull-down configuration.
 * @return ::HAL_OK on success.
 * @note Enables the pin's port clock automatically.
 */
hal_status_t hal_gpio_set_mode(hal_gpio_pin_t pin, hal_gpio_mode_t mode,
                               hal_gpio_pull_t pull);

/**
 * @brief Get the current mode of a pin.
 * @param pin Pin to query.
 * @return The pin's current ::hal_gpio_mode_t.
 */
hal_gpio_mode_t hal_gpio_get_mode(hal_gpio_pin_t pin);

/**
 * @brief Enable the peripheral clock for a pin's GPIO port.
 * @param pin Pin whose port clock is enabled.
 * @return ::HAL_OK on success.
 */
hal_status_t hal_gpio_enable_clock(hal_gpio_pin_t pin);

/**
 * @brief Select an alternate function for a pin (also sets mode to AF).
 * @param pin Pin to configure.
 * @param af  Alternate function to assign.
 * @return ::HAL_OK on success.
 */
hal_status_t hal_gpio_set_alternate_function(hal_gpio_pin_t pin,
                                             hal_gpio_af_t af);

/**
 * @brief Set a pin's output driver type (push-pull or open-drain).
 * @param pin  Pin to configure.
 * @param type Output type.
 * @return ::HAL_OK on success.
 */
hal_status_t hal_gpio_set_output_type(hal_gpio_pin_t pin,
                                      hal_gpio_output_type_t type);

/**
 * @brief Set a pin's output speed / slew rate.
 * @param pin   Pin to configure.
 * @param speed Output speed.
 * @return ::HAL_OK on success.
 */
hal_status_t hal_gpio_set_output_speed(hal_gpio_pin_t pin,
                                       hal_gpio_output_speed_t speed);

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Port-specific hot-path inlines + register-backed defines. */
#include "navhal_port_gpio.h"

#endif /* HAL_GPIO_H */
