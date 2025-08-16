/**
 * @file pwm.c
 * @brief PWM driver implementation for STM32F4 series (Cortex-M4)
 * @details
 * This file provides Pulse Width Modulation (PWM) functionality for STM32F4
 * microcontrollers. It supports:
 * - PWM generation on all timer channels
 * - Configurable frequency and duty cycle
 * - Automatic timer clock selection (APB1/APB2)
 * - Runtime duty cycle adjustment
 *
 * The implementation uses the STM32 timer peripherals to generate precise
 * PWM signals with minimal CPU overhead.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "core/cortex-m4/pwm.h"
#include "core/cortex-m4/clock.h"
#include "core/cortex-m4/timer.h"
#include <stdint.h>

/**
 * @brief Initialize PWM with specified frequency and duty cycle
 * @param[in,out] pwm Pointer to PWM handle structure
 * @param[in] frequency Desired PWM frequency in Hz
 * @param[in] dutyCycle Initial duty cycle (0.0 to 1.0)
 *
 * @details
 * Configures the timer peripheral to generate PWM signals by:
 * 1. Selecting the appropriate bus clock (APB1/APB2)
 * 2. Calculating prescaler and auto-reload values
 * 3. Setting the initial compare value for duty cycle
 * 4. Initializing the timer peripheral
 *
 * @note For advanced timers (TIM1, TIM9-TIM11), APB2 clock is used automatically
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
 * @brief Start PWM output
 * @param[in] pwm Pointer to initialized PWM handle structure
 *
 * @details
 * Enables the timer counter to start generating PWM signals on the
 * configured channel. The output will appear on the corresponding GPIO pin.
 */
void hal_pwm_start(PWM_Handle *pwm) { timer_start(pwm->timer); }

/**
 * @brief Stop PWM output
 * @param[in] pwm Pointer to PWM handle structure
 *
 * @details
 * Disables the timer channel output and stops the timer counter.
 * The PWM output pin will return to its idle state.
 */
void hal_pwm_stop(PWM_Handle *pwm) {
  timer_disable_channel(pwm->timer, pwm->channel);
  timer_stop(pwm->timer);
}

/**
 * @brief Set PWM duty cycle
 * @param[in,out] pwm Pointer to PWM handle structure
 * @param[in] dutyCycle New duty cycle (0.0 to 1.0)
 *
 * @details
 * Updates the capture/compare register to change the PWM duty cycle
 * while maintaining the current frequency. The change takes effect
 * immediately on the output.
 *
 * @note Duty cycle is clamped to valid range (0.0-1.0)
 */
void hal_pwm_set_duty_cycle(PWM_Handle *pwm, float dutyCycle) {
  uint32_t arr = timer_get_arr(pwm->timer, pwm->channel);
  uint32_t ccr = (uint32_t)((arr + 1) * dutyCycle + 0.5f);
  if (ccr > arr)
    ccr = arr;
  timer_set_compare(pwm->timer, pwm->channel, ccr);
}

/**
 * @brief Set PWM frequency (not implemented)
 * @param[in,out] pwm Pointer to PWM handle structure
 * @param[in] frequency Desired PWM frequency in Hz
 *
 * @note This function is currently not implemented
 * @todo Implement frequency change functionality
 */
void hal_pwm_set_frequency(PWM_Handle *pwm, uint32_t frequency) {
  /* Timer_SetFrequency(&pwm->timer, frequency); */
}