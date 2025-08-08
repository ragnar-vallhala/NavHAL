#ifndef PWM_H
#define PWM_H

#include "utils/timer_types.h"
#include <stdint.h>

typedef struct {
  hal_timer_t timer;
  uint32_t channel;
} PWM_Handle;

/**
 * @brief Initialize PWM on a specific timer channel
 * @param pwm PWM handle containing timer and channel
 * @param frequency PWM frequency in Hz
 * @param dutyCycle Duty cycle in percent (0–100)
 */
void hal_pwm_init(PWM_Handle *pwm, uint32_t frequency, float dutyCycle);

/**
 * @brief Start PWM signal generation
 * @param pwm PWM handle
 */
void hal_pwm_start(PWM_Handle *pwm);

/**
 * @brief Stop PWM signal generation
 * @param pwm PWM handle
 */
void hal_pwm_stop(PWM_Handle *pwm);

/**
 * @brief Update the duty cycle (0–100%)
 * @param pwm PWM handle
 * @param dutyCycle New duty cycle
 */
void hal_pwm_set_duty_cycle(PWM_Handle *pwm, float dutyCycle);

/**
 * @brief Update the PWM frequency (may affect resolution)
 * @param pwm PWM handle
 * @param frequency New frequency in Hz
 */
void hal_pwm_set_frequency(PWM_Handle *pwm, uint32_t frequency);

#endif // !PWM_H
