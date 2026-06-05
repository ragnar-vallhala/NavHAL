/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file pwm.c
 * @brief Standardized HAL PWM driver for STM32F4 (Cortex-M4).
 *
 * @details
 * Implements the standardized `hal_pwm_*` API declared in
 * `port/cortex-m4/navhal_port_pwm.h`. PWM signals are produced on a timer channel; this
 * driver computes the timer prescaler / auto-reload / compare values and
 * delegates to the standardized timer driver.
 */

#include "internal/hal_pwm_ops.h"
#include "navhal_port_clock.h"
#include "family/rcc_reg.h"
#include "navhal_port_timer.h"
#include <stdint.h>

static hal_status_t stm32_pwm_init(hal_pwm_handle_t *pwm, uint32_t frequency,
                                   float duty_cycle) {
  /* pwm non-NULL and frequency != 0: validated by the public layer. */
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

static hal_status_t stm32_pwm_start(hal_pwm_handle_t *pwm) {
  /* pwm non-NULL: validated by the public layer. */
  hal_timer_start(pwm->timer);
  return HAL_OK;
}

static hal_status_t stm32_pwm_stop(hal_pwm_handle_t *pwm) {
  /* pwm non-NULL: validated by the public layer. */
  hal_timer_disable_channel(pwm->timer, pwm->channel);
  hal_timer_stop(pwm->timer);
  return HAL_OK;
}

static hal_status_t stm32_pwm_set_duty_cycle(hal_pwm_handle_t *pwm,
                                             float duty_cycle) {
  /* pwm non-NULL: validated by the public layer. */
  uint32_t arr = hal_timer_get_auto_reload(pwm->timer);
  uint32_t ccr = (uint32_t)((float)(arr + 1) * duty_cycle + 0.5f);
  if (ccr > arr)
    ccr = arr;
  hal_timer_set_compare(pwm->timer, pwm->channel, ccr);
  return HAL_OK;
}

static hal_status_t stm32_pwm_set_frequency(hal_pwm_handle_t *pwm,
                                            uint32_t frequency) {
  /* pwm non-NULL: validated by the public layer. */
  (void)pwm;
  (void)frequency;
  return HAL_ERR_NOT_SUPPORTED; // not yet implemented
}

const hal_pwm_ops_t _hal_pwm_ops = {
    .init = stm32_pwm_init,
    .start = stm32_pwm_start,
    .stop = stm32_pwm_stop,
    .set_duty_cycle = stm32_pwm_set_duty_cycle,
    .set_frequency = stm32_pwm_set_frequency,
};
