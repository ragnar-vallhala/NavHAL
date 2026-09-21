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
 * @file timer.c
 * @brief General-purpose timer driver for STM32F4 (TIMx peripherals).
 *
 * @details
 * Timer peripheral base lookup, RCC enable, init/start/stop/reset; timer
 * interrupt control, attach/detach callbacks and IRQ handlers; channel
 * compare and PWM setup helpers.
 *
 * The SysTick-backed timebase that this file used to host has moved to the
 * arch layer at `src/arch/armv7e-m/timebase/timebase.c` — it is core-bound,
 * not vendor-bound, so it lives with the other ARMv7E-M arch code.
 */

#include "internal/hal_timer_ops.h"
#include "navhal_port_timer.h"
#include "navhal_port_clock.h"
#include "navhal_port_interrupt.h"
#include "family/rcc_reg.h"
#include "family/timer_reg.h"
#include "utils/timer_types.h"
#include <stdint.h>

/* Forward declaration: set_compare enables the channel, defined further down. */
static hal_status_t stm32_timer_set_channel_enabled(hal_timer_t timer,
                                                    uint32_t channel, bool on);

/**
 * @internal
 * @brief Enable the peripheral clock for the given timer in RCC registers.
 * @param timer Timer identifier.
 */
static void _enable_timer_rcc(hal_timer_t timer) {
  switch (timer) {
  case TIM1:
    RCC->APB2ENR |= (1 << RCC_APB2ENR_TIM1_OFFSET);
    break;
  case TIM2:
    RCC->APB1ENR |= (1 << RCC_APB1ENR_TIM2_OFFSET);
    break;
  case TIM3:
    RCC->APB1ENR |= (1 << RCC_APB1ENR_TIM3_OFFSET);
    break;
  case TIM4:
    RCC->APB1ENR |= (1 << RCC_APB1ENR_TIM4_OFFSET);
    break;
  case TIM5:
    RCC->APB1ENR |= (1 << RCC_APB1ENR_TIM5_OFFSET);
    break;
  case TIM9:
    RCC->APB2ENR |= (1 << RCC_APB2ENR_TIM9_OFFSET);
    break;
  case TIM10:
    RCC->APB2ENR |= (1 << RCC_APB2ENR_TIM10_OFFSET);
    break;
  case TIM11:
    RCC->APB2ENR |= (1 << RCC_APB2ENR_TIM11_OFFSET);
    break;
  default:
    break;
  }
}

/**
 * @brief Initialize a timer with a prescaler and auto-reload value.
 *
 * @param timer Timer identifier.
 * @param cfg   Configuration (prescaler + auto-reload); must not be NULL.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG if @p cfg is NULL or the timer
 *         is invalid.
 */
static hal_status_t stm32_timer_init(hal_timer_t timer,
                                     const hal_timer_config_t *cfg) {
  /* cfg is non-NULL: the public layer validated it before dispatching. */
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return HAL_ERR_INVALID_ARG;

  uint32_t auto_reload = cfg->auto_reload;
  // Enable clock
  _enable_timer_rcc(timer);
  tim->CR1 &= (~(TIMx_CR1_CEN));
  tim->PSC = cfg->prescaler;
  tim->CNT = 0;
  if (!(timer == TIM2 || timer == TIM5))
    auto_reload = (uint16_t)auto_reload;
  tim->ARR = auto_reload;
  tim->EGR |= TIMx_EGR_UG;
  tim->CR1 |= TIMx_CR1_CEN;
  return HAL_OK;
}

/**
 * @brief Initialize a timer to a specific update frequency.
 *
 * @param timer Timer identifier.
 * @param freq  Desired frequency in Hz; must be non-zero.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG if @p freq is 0 or the timer is
 *         invalid.
 *
 * @note Computes the optimal PSC and ARR to achieve the target frequency,
 *       handling both 16-bit and 32-bit timers.
 */
/* APB timer clock: the bus clock, doubled when its prescaler is not /1. */
static uint32_t stm32_timer_get_input_clock(hal_timer_t timer) {
  uint32_t ppre1 = (RCC->CFGR >> RCC_CFGR_PPRE1_BIT) & 0x7;
  uint32_t ppre2 = (RCC->CFGR >> RCC_CFGR_PPRE2_BIT) & 0x7;

  if (timer == TIM1 || timer == TIM9 || timer == TIM10 || timer == TIM11) {
    uint32_t apb2 = hal_clock_get_apb2clk();
    return (ppre2 == 0) ? apb2 : (apb2 * 2);
  }
  uint32_t apb1 = hal_clock_get_apb1clk();
  return (ppre1 == 0) ? apb1 : (apb1 * 2);
}

/* Split `ticks` into the PSC/ARR pair that expresses it. TIM2/TIM5 are 32-bit
 * counters; the rest are 16-bit, which is why a prescaler search is needed at
 * all. The shared layer has already turned a frequency into ticks. */
static hal_status_t stm32_timer_set_timebase(hal_timer_t timer,
                                             uint64_t total_ticks) {
  if (total_ticks == 0u)
    total_ticks = 1u;

  uint32_t psc = 0;
  uint32_t arr = 0;

  if (timer == TIM2 || timer == TIM5) {
    if (total_ticks > 0xFFFFFFFFULL) {
      psc = (uint32_t)(total_ticks / 0xFFFFFFFFULL);
      arr = (uint32_t)(total_ticks / (psc + 1)) - 1;
    } else {
      psc = 0;
      arr = (uint32_t)total_ticks - 1;
    }
  } else {
    if (total_ticks > 0x10000ULL) {
      /* Smallest PSC whose ARR still fits 16 bits, then a short search for a
       * pair that divides `total_ticks` more exactly. */
      uint32_t min_psc = (uint32_t)(total_ticks / 0x10000ULL);
      uint32_t best_psc = min_psc;
      uint32_t best_arr = (uint32_t)(total_ticks / (min_psc + 1)) - 1;
      uint64_t min_error = total_ticks % (min_psc + 1);

      for (uint32_t pp = min_psc; pp < min_psc + 10 && pp <= 0xFFFF; pp++) {
        uint64_t error = total_ticks % (pp + 1);
        if (error < min_error) {
          min_error = error;
          best_psc = pp;
          best_arr = (uint32_t)(total_ticks / (pp + 1)) - 1;
        }
        if (error == 0)
          break;
      }
      psc = best_psc;
      arr = best_arr;
    } else {
      psc = 0;
      arr = (uint32_t)total_ticks - 1;
    }
  }

  hal_timer_config_t cfg = {.prescaler = psc, .auto_reload = arr};
  return stm32_timer_init(timer, &cfg);
}

/**
 * @brief Start the specified timer.
 * @param timer Timer identifier.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an invalid timer.
 */
static hal_status_t stm32_timer_set_running(hal_timer_t timer, bool on) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return HAL_ERR_INVALID_ARG;
  if (on)
    tim->CR1 |= TIMx_CR1_CEN;
  else
    tim->CR1 &= (~TIMx_CR1_CEN);
  return HAL_OK;
}

/**
 * @brief Stop the specified timer.
 * @param timer Timer identifier.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an invalid timer.
 */

/**
 * @brief Reset the timer counter to zero.
 * @param timer Timer identifier.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an invalid timer.
 */
static hal_status_t stm32_timer_reset(hal_timer_t timer) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return HAL_ERR_INVALID_ARG;
  tim->CNT = 0;
  return HAL_OK;
}

/**
 * @brief Get the current counter value of a timer.
 * @param timer Timer identifier.
 * @return Current counter value, or 0 for an invalid timer.
 */
static uint32_t stm32_timer_get_count(hal_timer_t timer) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return 0;
  return tim->CNT;
}

/**
 * @brief Calculate a timer's current base frequency from PSC and ARR.
 * @param timer Timer identifier.
 * @return Timer frequency in Hz, or 0 for an invalid timer.
 */
/* Effective divider, not the register value: PSC divides by PSC+1. */
static uint32_t stm32_timer_get_divider(hal_timer_t timer) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return 0;
  return tim->PSC + 1u;
}

/**
 * @brief Set a timer's prescaler (PSC) register.
 * @param timer     Timer identifier.
 * @param prescaler Prescaler value.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an invalid timer.
 */
/* PSC divides by PSC+1, so an N-way divide is PSC = N-1. */
static hal_status_t stm32_timer_set_divider(hal_timer_t timer,
                                            uint32_t divider) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL || divider == 0u || divider > 0x10000u)
    return HAL_ERR_INVALID_ARG;
  tim->PSC = (uint16_t)(divider - 1u);
  return HAL_OK;
}

static hal_status_t stm32_timer_set_prescaler(hal_timer_t timer, uint32_t prescaler) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return HAL_ERR_INVALID_ARG;
  tim->PSC = prescaler;
  return HAL_OK;
}

/**
 * @brief Set a timer's auto-reload (ARR) register.
 * @param timer       Timer identifier.
 * @param auto_reload Auto-reload value (clamped to 16 bits for 16-bit timers).
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an invalid timer.
 */
static hal_status_t stm32_timer_set_auto_reload(hal_timer_t timer,
                                       uint32_t auto_reload) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return HAL_ERR_INVALID_ARG;

  stm32_timer_set_running(timer, false);
  if (!(timer == TIM2 || timer == TIM5))
    auto_reload = (uint16_t)auto_reload;
  tim->ARR = auto_reload;
  stm32_timer_set_running(timer, true);
  return HAL_OK;
}

/**
 * @brief Get a timer's auto-reload (ARR) register value.
 * @param timer Timer identifier.
 * @return Auto-reload value, or 0 for an invalid timer.
 */
static uint32_t stm32_timer_get_auto_reload(hal_timer_t timer) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return 0;
  return tim->ARR;
}

/**
 * @brief Clear a timer's update interrupt flag.
 * @param timer Timer identifier.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an invalid timer.
 */
static hal_status_t stm32_timer_clear_interrupt_flag(hal_timer_t timer) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return HAL_ERR_INVALID_ARG;
  tim->SR &= ~TIMx_SR_UIF;
  return HAL_OK;
}

/** @brief IRQ handler wrapper for TIM2. */
void TIM2_IRQHandler(void) {
  stm32_timer_clear_interrupt_flag(TIM2);
  hal_interrupt_dispatch(TIM2_IRQn);
}

/** @brief IRQ handler wrapper for TIM3. */
void TIM3_IRQHandler(void) {
  stm32_timer_clear_interrupt_flag(TIM3);
  hal_interrupt_dispatch(TIM3_IRQn);
}

/** @brief IRQ handler wrapper for TIM4. */
void TIM4_IRQHandler(void) {
  stm32_timer_clear_interrupt_flag(TIM4);
  hal_interrupt_dispatch(TIM4_IRQn);
}

/** @brief IRQ handler wrapper for TIM5. */
void TIM5_IRQHandler(void) {
  stm32_timer_clear_interrupt_flag(TIM5);
  hal_interrupt_dispatch(TIM5_IRQn);
}

/**
 * @brief IRQ handler for the shared TIM1-BRK / TIM9 vector.
 * @note Clears TIM9's flag and dispatches using the shared IRQn.
 */
void TIM1BRK_TIM9_IRQHandler(void) {
  stm32_timer_clear_interrupt_flag(TIM9);
  hal_interrupt_dispatch(TIM1_BRK_TIM9_IRQn); // shared with TIM1 BRK
}

/**
 * @internal
 * @brief Set the update-interrupt-enable bit (DIER UIE) for a timer.
 * @param timer Timer identifier.
 */
static void _set_interrupt_enable_bit(hal_timer_t timer) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return;
  tim->DIER |= TIMx_DIER_UIE;
}

/**
 * @brief Enable a timer's update interrupt (NVIC + DIER UIE).
 * @param timer Timer identifier.
 * @return ::HAL_OK.
 * @note TIM1's more complex interrupt options are not yet implemented.
 */
/* Every interrupt entry point below needs the same timer -> IRQ line map, so
 * it lives here once. TIM1 splits update/break/trigger across separate IRQs
 * and is not wired up. */
static bool _timer_irq(hal_timer_t timer, hal_irq_t *out) {
  switch (timer) {
  case TIM2:
    *out = TIM2_IRQn;
    return true;
  case TIM3:
    *out = TIM3_IRQn;
    return true;
  case TIM4:
    *out = TIM4_IRQn;
    return true;
  case TIM5:
    *out = TIM5_IRQn;
    return true;
  case TIM9:
    *out = TIM1_BRK_TIM9_IRQn;
    return true;
  default:
    return false;
  }
}

static hal_status_t stm32_timer_set_interrupt(hal_timer_t timer, bool on) {
  hal_irq_t irq;
  if (_timer_irq(timer, &irq)) {
    if (on)
      hal_interrupt_enable(irq);
    else
      hal_interrupt_disable(irq);
  }

  if (on) {
    _set_interrupt_enable_bit(timer);
  } else {
    TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
    if (tim != NULL)
      tim->DIER &= ~TIMx_DIER_UIE;
  }
  return HAL_OK;
}

static hal_status_t stm32_timer_set_callback(hal_timer_t timer,
                                             hal_timer_callback_t callback) {
  hal_irq_t irq;
  if (!_timer_irq(timer, &irq))
    return HAL_OK;

  if (callback != NULL)
    hal_interrupt_attach_callback(irq, callback);
  else
    hal_interrupt_detach_callback(irq);
  return HAL_OK;
}


/**
 * @brief Disable a timer's update interrupt (NVIC + DIER UIE).
 * @param timer Timer identifier.
 * @return ::HAL_OK.
 */

/**
 * @brief Register a callback for a timer's update interrupt.
 * @param timer    Timer identifier.
 * @param callback Callback to invoke, or NULL to clear.
 * @return ::HAL_OK.
 */

/**
 * @brief Remove the callback registered for a timer's update interrupt.
 * @param timer Timer identifier.
 * @return ::HAL_OK.
 */

/**
 * @brief Set a channel's compare register and configure it for PWM mode 1.
 *
 * @param timer         Timer identifier.
 * @param channel       Channel number (1-4).
 * @param compare_value Value to write into CCRx.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an invalid timer/channel.
 */
static hal_status_t stm32_timer_set_compare(hal_timer_t timer, uint8_t channel,
                                   uint32_t compare_value) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return HAL_ERR_INVALID_ARG;
  if (channel < 1 || channel > 4)
    return HAL_ERR_INVALID_ARG; // only 4 valid channels
  switch (channel) {
  case 1:
    tim->CCR1 = compare_value;
    break;
  case 2:
    tim->CCR2 = compare_value;
    break;
  case 3:
    tim->CCR3 = compare_value;
    break;
  case 4:
    tim->CCR4 = compare_value;
    break;
  default:
    break;
  }
  // Correctly handle CCMRx bitfields
  if (channel <= 2) {
    // Clear OCxM bits
    tim->CCMR1 &= ~TIMx_CCMRy_OCzM_MASK(channel);
    // Set PWM Mode 1 (0x6) and Enable Preload
    tim->CCMR1 |= TIMx_CCMRy_OCzM_PWM_MODE1_MASK(channel);
    tim->CCMR1 |= TIMx_CCMRy_OCxPE(channel);
  } else {
    // Clear OCxM bits
    tim->CCMR2 &= ~TIMx_CCMRy_OCzM_MASK(channel);
    // Set PWM Mode 1 (0x6) and Enable Preload
    tim->CCMR2 |= TIMx_CCMRy_OCzM_PWM_MODE1_MASK(channel);
    tim->CCMR2 |= TIMx_CCMRy_OCxPE(channel);
  }
  stm32_timer_set_channel_enabled(timer, channel, true);
  return HAL_OK;
}

/**
 * @brief Get a channel's compare register value.
 * @param timer   Timer identifier.
 * @param channel Channel number (1-4).
 * @return Compare value, or 0 for an invalid timer/channel.
 */
static uint32_t stm32_timer_get_compare(hal_timer_t timer, uint32_t channel) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL || channel < 1 || channel > 4)
    return 0;
  switch (channel) {
  case 1:
    return tim->CCR1;
  case 2:
    return tim->CCR2;
  case 3:
    return tim->CCR3;
  case 4:
    return tim->CCR4;
  default:
    return 0;
  }
}

/**
 * @brief Enable output on a timer channel (CCxE = 1).
 * @param timer   Timer identifier.
 * @param channel Channel number (1-4).
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an invalid timer/channel.
 */
static hal_status_t stm32_timer_set_channel_enabled(hal_timer_t timer,
                                                    uint32_t channel, bool on) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL || channel < 1 || channel > 4)
    return HAL_ERR_INVALID_ARG;
  if (on) {
    tim->CCER |= TIMx_CCER_CCxE_MASK(channel);
    /* MOE gates all outputs on the advanced timer; left set on disable, as
     * before, so disabling one channel does not kill the others. */
    if (timer == TIM1)
      tim->BDTR |= TIMx_BDTR_MOE;
  } else {
    tim->CCER &= (~TIMx_CCER_CCxE_MASK(channel));
  }
  return HAL_OK;
}

/**
 * @brief Disable output on a timer channel (CCxE = 0).
 * @param timer   Timer identifier.
 * @param channel Channel number (1-4).
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an invalid timer/channel.
 */

const hal_timer_ops_t _hal_timer_ops = {
    .init = stm32_timer_init,
    .set_timebase = stm32_timer_set_timebase,
    .get_input_clock = stm32_timer_get_input_clock,
    .set_running = stm32_timer_set_running,
    .reset = stm32_timer_reset,
    .get_count = stm32_timer_get_count,
    .set_prescaler = stm32_timer_set_prescaler,
    .set_divider = stm32_timer_set_divider,
    .get_divider = stm32_timer_get_divider,
    .set_auto_reload = stm32_timer_set_auto_reload,
    .get_auto_reload = stm32_timer_get_auto_reload,
    .set_interrupt = stm32_timer_set_interrupt,
    .clear_interrupt_flag = stm32_timer_clear_interrupt_flag,
    .set_callback = stm32_timer_set_callback,
    .set_channel_enabled = stm32_timer_set_channel_enabled,
    .set_compare = stm32_timer_set_compare,
    .get_compare = stm32_timer_get_compare,
};
