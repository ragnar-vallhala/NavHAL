#ifndef PWM_H
#define PWM_H

#include "utils/timer_types.h"
#include <stdint.h>

/**
 * @file pwm.h
 * @brief Handle structure for PWM (Pulse Width Modulation) configuration.
 */
typedef struct {
  hal_timer_t timer;   /**< Hardware timer used for PWM generation. */
  uint32_t channel;    /**< PWM channel number associated with the timer. */
} PWM_Handle;

/**
 * @brief Initialize a PWM handle with the specified frequency and duty cycle.
 *
 * @param pwm Pointer to the PWM handle.
 * @param frequency PWM frequency in Hz.
 * @param dutyCycle Duty cycle as a percentage (0.0 to 100.0).
 */
void hal_pwm_init(PWM_Handle *pwm, uint32_t frequency, float dutyCycle);

/**
 * @brief Start PWM signal generation.
 *
 * @param pwm Pointer to the PWM handle.
 */
void hal_pwm_start(PWM_Handle *pwm);

/**
 * @brief Stop PWM signal generation.
 *
 * @param pwm Pointer to the PWM handle.
 */
void hal_pwm_stop(PWM_Handle *pwm);

/**
 * @brief Set the PWM duty cycle.
 *
 * @param pwm Pointer to the PWM handle.
 * @param dutyCycle Duty cycle as a percentage (0.0 to 100.0).
 */
void hal_pwm_set_duty_cycle(PWM_Handle *pwm, float dutyCycle);

/**
 * @brief Set the PWM frequency.
 *
 * @param pwm Pointer to the PWM handle.
 * @param frequency PWM frequency in Hz.
 */
void hal_pwm_set_frequency(PWM_Handle *pwm, uint32_t frequency);

#endif // !PWM_H
