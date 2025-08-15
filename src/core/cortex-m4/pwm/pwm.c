/**
 * @file pwm.c
 * @brief PWM driver implementation for STM32F4 series (Cortex-M4).
 */

#include "core/cortex-m4/pwm.h"
#include "core/cortex-m4/clock.h"
#include "core/cortex-m4/timer.h"
#include <stdint.h>

/**
 * @brief Initialize PWM with a specified frequency and duty cycle.
 *
 * This function configures the timer to generate a PWM signal on the
 * specified channel with the given frequency and duty cycle.
 *
 * @param pwm Pointer to the PWM handle structure.
 * @param frequency Desired PWM frequency in Hz.
 * @param dutyCycle Duty cycle as a fraction (0.0f - 1.0f).
 */
void hal_pwm_init(PWM_Handle *pwm, uint32_t frequency, float dutyCycle) {

  // 1. Get clock
  uint32_t bus_clk = hal_clock_get_apb1clk(); // default for TIM2-TIM5
  if (pwm->timer == TIM1 || pwm->timer == TIM9 || pwm->timer == TIM10 ||
      pwm->timer == TIM11) {
    bus_clk = hal_clock_get_apb2clk(); // For advanced timers
  }
  uint32_t psc = bus_clk / 1e6 - 1;
  uint32_t arr = (bus_clk / (psc + 1)) / frequency - 1;
  uint32_t ccr = (uint32_t)((arr + 1) * dutyCycle + 0.5f);
  if (ccr > arr)
    ccr = arr;
  timer_init(pwm->timer, psc, arr);
  timer_set_compare(pwm->timer, pwm->channel, ccr);
}

/**
 * @brief Start PWM output.
 *
 * @param pwm Pointer to the PWM handle structure.
 */
void hal_pwm_start(PWM_Handle *pwm) { timer_start(pwm->timer); }

/**
 * @brief Stop PWM output.
 *
 * This function disables the timer channel and stops the timer.
 *
 * @param pwm Pointer to the PWM handle structure.
 */
void hal_pwm_stop(PWM_Handle *pwm) {
  timer_disable_channel(pwm->timer, pwm->channel);
  timer_stop(pwm->timer);
}

/**
 * @brief Set PWM duty cycle.
 *
 * @param pwm Pointer to the PWM handle structure.
 * @param dutyCycle New duty cycle as a fraction (0.0f - 1.0f).
 */
void hal_pwm_set_duty_cycle(PWM_Handle *pwm, float dutyCycle) {
  uint32_t arr = timer_get_arr(pwm->timer, pwm->channel);
  uint32_t ccr = (uint32_t)((arr + 1) * dutyCycle + 0.5f);
  if (ccr > arr)
    ccr = arr;
  timer_set_compare(pwm->timer, pwm->channel, ccr);
}

/**
 * @brief Set PWM frequency.
 *
 * @note This function is not yet implemented.
 *
 * @param pwm Pointer to the PWM handle structure.
 * @param frequency Desired PWM frequency in Hz.
 */
void hal_pwm_set_frequency(PWM_Handle *pwm, uint32_t frequency) {
  /* Timer_SetFrequency(&pwm->timer, frequency); */
}
