/**
 * @file hal_pwm.h
 * @brief Portable HAL interface for Pulse Width Modulation (PWM).
 *
 * @details
 * Standardized PWM API (see @c docs/api_standardization.md). PWM is generated
 * on a timer channel; a ::hal_pwm_handle_t binds the timer and channel, and
 * is the first argument to every @c hal_pwm_* function. Functions return
 * ::hal_status_t.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef HAL_PWM_H
#define HAL_PWM_H

#include "common/hal_status.h"
#include "common/navhal_compiler.h"
#include "utils/timer_types.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief PWM handle binding a hardware timer and channel.
 */
typedef struct {
  hal_timer_t timer; /**< Hardware timer used for PWM generation. */
  uint32_t channel;  /**< Timer channel (1-4) used for the PWM output. */
} hal_pwm_handle_t;

/**
 * @brief Initialize a PWM output with the given frequency and duty cycle.
 * @param pwm        PWM handle (timer + channel); must not be NULL.
 * @param frequency  PWM frequency in Hz; must be non-zero.
 * @param duty_cycle Duty cycle as a fraction (0.0 - 1.0).
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for a NULL handle / zero frequency.
 */
hal_status_t hal_pwm_init(hal_pwm_handle_t *pwm, uint32_t frequency,
                          float duty_cycle);

/**
 * @brief Start PWM signal generation.
 * @param pwm PWM handle; must not be NULL.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG.
 */
hal_status_t hal_pwm_start(hal_pwm_handle_t *pwm);

/**
 * @brief Stop PWM signal generation.
 * @param pwm PWM handle; must not be NULL.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG.
 */
hal_status_t hal_pwm_stop(hal_pwm_handle_t *pwm);

/**
 * @brief Set the PWM duty cycle.
 * @param pwm        PWM handle; must not be NULL.
 * @param duty_cycle Duty cycle as a fraction (0.0 - 1.0).
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG.
 */
hal_status_t hal_pwm_set_duty_cycle(hal_pwm_handle_t *pwm, float duty_cycle);

/**
 * @brief Set the PWM frequency.
 * @param pwm       PWM handle; must not be NULL.
 * @param frequency PWM frequency in Hz.
 * @return ::HAL_ERR_NOT_SUPPORTED — not yet implemented.
 */
hal_status_t hal_pwm_set_frequency(hal_pwm_handle_t *pwm, uint32_t frequency);

/* Deprecated pre-standardization PWM handle type — retained as a backward-compat alias. */
typedef hal_pwm_handle_t PWM_Handle NAVHAL_DEPRECATED("use hal_pwm_handle_t");

#ifdef __cplusplus
} /* extern "C" */
#endif

#include "navhal_port_pwm.h"

#endif /* HAL_PWM_H */
