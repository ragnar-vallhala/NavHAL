/**
 * @file core/cortex-m4/gpio.h
 * @brief Cortex-M4 / STM32F4 GPIO HAL driver interface.
 *
 * @details
 * Standardized GPIO API (see `docs/api_standardization.md`). All public
 * functions use the `hal_gpio_` prefix and `snake_case` verbs; configuration
 * operations return ::hal_status_t, while the hot-path pin write/read/toggle
 * helpers are `static inline` and return their value directly.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef CORTEX_M4_GPIO_H
#define CORTEX_M4_GPIO_H

#include "common/hal_status.h"
#include "core/cortex-m4/gpio_reg.h"
#include "utils/gpio_types.h"


#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Aggregate configuration for a single GPIO pin.
 *
 * Passed to ::hal_gpio_init. The `output_type` and `output_speed` fields apply
 * only when @ref mode is ::HAL_GPIO_MODE_OUTPUT or ::HAL_GPIO_MODE_AF; the
 * `alternate` field applies only when @ref mode is ::HAL_GPIO_MODE_AF.
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

/**
 * @brief Write a logic level to a pin (hot path).
 * @param pin   Pin to drive.
 * @param state Logic level to apply.
 */
static inline void hal_gpio_write(hal_gpio_pin_t pin, hal_gpio_state_t state) {
  if (state)
    GPIO_GET_PORT(pin)->BSRR = (1U << GPIO_GET_PIN(pin));
  else
    GPIO_GET_PORT(pin)->BSRR = (1U << (GPIO_GET_PIN(pin) + 16));
}

/**
 * @brief Read the logic level of a pin (hot path).
 * @param pin Pin to read.
 * @return The pin's current ::hal_gpio_state_t.
 */
static inline hal_gpio_state_t hal_gpio_read(hal_gpio_pin_t pin) {
  return (hal_gpio_state_t)((GPIO_GET_PORT(pin)->IDR >> GPIO_GET_PIN(pin)) &
                            0x1);
}

/**
 * @brief Toggle a pin's output level (hot path).
 * @param pin Pin to toggle.
 */
static inline void hal_gpio_toggle(hal_gpio_pin_t pin) {
  GPIO_GET_PORT(pin)->ODR ^= (1U << GPIO_GET_PIN(pin));
}

/* Deprecated pre-standardization GPIO names — removed in M5. */
#include "compat/gpio_compat.h"


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // CORTEX_M4_GPIO_H
