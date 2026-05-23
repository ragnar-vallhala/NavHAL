/**
 * @file pwm.c
 * @brief Standardized HAL PWM driver for STM32F4 (Cortex-M4).
 *
 * @details
 * Implements the standardized `hal_pwm_*` API declared in
 * `port/cortex-m4/navhal_port_pwm.h`. PWM signals are produced on a timer channel; this
 * driver computes the timer prescaler / auto-reload / compare values and
 * delegates to the standardized timer driver.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "navhal_port_pwm.h"
#include "navhal_port_clock.h"
#include "family/rcc_reg.h"
#include "navhal_port_timer.h"
#include <stdint.h>

hal_status_t hal_pwm_init(hal_pwm_handle_t *pwm, uint32_t frequency,
                          float duty_cycle) {
  if (pwm == NULL || frequency == 0)
    return HAL_ERR_INVALID_ARG;

  // 1. Get clock
  uint32_t bus_clk = hal_clock_get_apb1clk(); // default for TIM2-TIM5
  uint32_t ppre = ((RCC->CFGR) >> RCC_CFGR_PPRE1_BIT) & 0x7;
  if (pwm->timer == TIM1 || pwm->timer == TIM9 || pwm->timer == TIM10 ||
      pwm->timer == TIM11) {
    bus_clk = hal_clock_get_apb2clk(); // For advanced timers
    ppre = ((RCC->CFGR) >> RCC_CFGR_PPRE2_BIT) & 0x7;
  }

  // STM32 Timer Clock Rule: If APB prescaler is 1, timer clock = APB clock.
  // Otherwise, timer clock = 2 * APB clock.
  uint32_t timer_clk = bus_clk;
  if (ppre != 0) { // 0 is DIV1
    timer_clk *= 2;
  }

  uint32_t psc = timer_clk / 1000000 - 1;
  uint32_t arr = (timer_clk / (psc + 1)) / frequency - 1;
  uint32_t ccr = (uint32_t)((float)(arr + 1) * duty_cycle + 0.5f);
  if (ccr > arr)
    ccr = arr;

  hal_timer_config_t cfg = {.prescaler = psc, .auto_reload = arr};
  hal_timer_init(pwm->timer, &cfg);
  hal_timer_set_compare(pwm->timer, pwm->channel, ccr);
  return HAL_OK;
}

hal_status_t hal_pwm_start(hal_pwm_handle_t *pwm) {
  if (pwm == NULL)
    return HAL_ERR_INVALID_ARG;
  hal_timer_start(pwm->timer);
  return HAL_OK;
}

hal_status_t hal_pwm_stop(hal_pwm_handle_t *pwm) {
  if (pwm == NULL)
    return HAL_ERR_INVALID_ARG;
  hal_timer_disable_channel(pwm->timer, pwm->channel);
  hal_timer_stop(pwm->timer);
  return HAL_OK;
}

hal_status_t hal_pwm_set_duty_cycle(hal_pwm_handle_t *pwm, float duty_cycle) {
  if (pwm == NULL)
    return HAL_ERR_INVALID_ARG;
  uint32_t arr = hal_timer_get_auto_reload(pwm->timer);
  uint32_t ccr = (uint32_t)((float)(arr + 1) * duty_cycle + 0.5f);
  if (ccr > arr)
    ccr = arr;
  hal_timer_set_compare(pwm->timer, pwm->channel, ccr);
  return HAL_OK;
}

hal_status_t hal_pwm_set_frequency(hal_pwm_handle_t *pwm, uint32_t frequency) {
  if (pwm == NULL)
    return HAL_ERR_INVALID_ARG;
  (void)frequency;
  return HAL_ERR_NOT_SUPPORTED; // not yet implemented
}
