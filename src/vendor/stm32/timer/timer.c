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
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "navhal_port_timer.h"
#include "navhal_port_clock.h"
#include "navhal_port_interrupt.h"
#include "family/rcc_reg.h"
#include "family/timer_reg.h"
#include "utils/timer_types.h"
#include <stdint.h>

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
hal_status_t hal_timer_init(hal_timer_t timer, const hal_timer_config_t *cfg) {
  if (cfg == NULL)
    return HAL_ERR_INVALID_ARG;
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
hal_status_t hal_timer_init_freq(hal_timer_t timer, uint32_t freq) {
  if (freq == 0)
    return HAL_ERR_INVALID_ARG;

  // 1. Get clock
  uint32_t timer_clk;
  uint32_t ppre1 = (RCC->CFGR >> RCC_CFGR_PPRE1_BIT) & 0x7;
  uint32_t ppre2 = (RCC->CFGR >> RCC_CFGR_PPRE2_BIT) & 0x7;

  if (timer == TIM1 || timer == TIM9 || timer == TIM10 || timer == TIM11) {
    uint32_t apb2 = hal_clock_get_apb2clk();
    timer_clk = (ppre2 == 0) ? apb2 : (apb2 * 2);
  } else {
    uint32_t apb1 = hal_clock_get_apb1clk();
    timer_clk = (ppre1 == 0) ? apb1 : (apb1 * 2);
  }

  // 2. Calculate ticks = timer_clk / freq
  uint64_t total_ticks = (uint64_t)timer_clk / freq;

  if (total_ticks == 0) {
    total_ticks = 1;
  }

  uint32_t psc = 0;
  uint32_t arr = 0;

  // For 32-bit timers (TIM2, TIM5), ARR can be up to 0xFFFFFFFF
  if (timer == TIM2 || timer == TIM5) {
    if (total_ticks > 0xFFFFFFFFULL) {
      psc = (uint32_t)(total_ticks / 0xFFFFFFFFULL);
      arr = (uint32_t)(total_ticks / (psc + 1)) - 1;
    } else {
      psc = 0;
      arr = (uint32_t)total_ticks - 1;
    }
  } else {
    // 16-bit timers (TIM1, TIM3, TIM4, TIM9, TIM10, TIM11)
    if (total_ticks > 0x10000ULL) {
      // Find PSC and ARR such that (PSC+1)*(ARR+1) is closest to total_ticks
      // We can iterate over PSC values or use a clever heuristic.
      // Smallest possible PSC is total_ticks / 65536
      uint32_t min_psc = (uint32_t)(total_ticks / 0x10000ULL);
      uint32_t best_psc = min_psc;
      uint32_t best_arr = (uint32_t)(total_ticks / (min_psc + 1)) - 1;
      uint64_t min_error = total_ticks % (min_psc + 1);

      // Simple search for better PSC if remainder is large
      for (uint32_t p = min_psc; p < min_psc + 10 && p <= 0xFFFF; p++) {
        uint64_t error = total_ticks % (p + 1);
        if (error < min_error) {
          min_error = error;
          best_psc = p;
          best_arr = (uint32_t)(total_ticks / (p + 1)) - 1;
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
  return hal_timer_init(timer, &cfg);
}

/**
 * @brief Start the specified timer.
 * @param timer Timer identifier.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an invalid timer.
 */
hal_status_t hal_timer_start(hal_timer_t timer) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return HAL_ERR_INVALID_ARG;
  tim->CR1 |= TIMx_CR1_CEN;
  return HAL_OK;
}

/**
 * @brief Stop the specified timer.
 * @param timer Timer identifier.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an invalid timer.
 */
hal_status_t hal_timer_stop(hal_timer_t timer) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return HAL_ERR_INVALID_ARG;
  tim->CR1 &= (~TIMx_CR1_CEN);
  return HAL_OK;
}

/**
 * @brief Reset the timer counter to zero.
 * @param timer Timer identifier.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an invalid timer.
 */
hal_status_t hal_timer_reset(hal_timer_t timer) {
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
uint32_t hal_timer_get_count(hal_timer_t timer) {
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
uint32_t hal_timer_get_frequency(hal_timer_t timer) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return 0;

  // 1. Get clock
  uint32_t timer_clk;

  uint32_t ppre1 = (RCC->CFGR >> RCC_CFGR_PPRE1_BIT) & 0x7;
  uint32_t ppre2 = (RCC->CFGR >> RCC_CFGR_PPRE2_BIT) & 0x7;

  if (timer == TIM1 || timer == TIM9 || timer == TIM10 || timer == TIM11) {
    uint32_t apb2 = hal_clock_get_apb2clk();
    timer_clk = (ppre2 == 0) ? apb2 : (apb2 * 2);
  } else {
    uint32_t apb1 = hal_clock_get_apb1clk();
    timer_clk = (ppre1 == 0) ? apb1 : (apb1 * 2);
  }

  // 2. Get prescaler and ARR
  uint32_t prescaler = tim->PSC;
  uint32_t arr = tim->ARR;

  if (arr == 0x0)
    arr = 0xFFFF; // Prevent division by zero

  // 3. Calculate frequency
  uint32_t freq = timer_clk / (prescaler + 1) / (arr + 1);
  return freq;
}

/**
 * @brief Set a timer's prescaler (PSC) register.
 * @param timer     Timer identifier.
 * @param prescaler Prescaler value.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an invalid timer.
 */
hal_status_t hal_timer_set_prescaler(hal_timer_t timer, uint32_t prescaler) {
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
hal_status_t hal_timer_set_auto_reload(hal_timer_t timer,
                                       uint32_t auto_reload) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return HAL_ERR_INVALID_ARG;

  hal_timer_stop(timer);
  if (!(timer == TIM2 || timer == TIM5))
    auto_reload = (uint16_t)auto_reload;
  tim->ARR = auto_reload;
  hal_timer_start(timer);
  return HAL_OK;
}

/**
 * @brief Get a timer's auto-reload (ARR) register value.
 * @param timer Timer identifier.
 * @return Auto-reload value, or 0 for an invalid timer.
 */
uint32_t hal_timer_get_auto_reload(hal_timer_t timer) {
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
hal_status_t hal_timer_clear_interrupt_flag(hal_timer_t timer) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return HAL_ERR_INVALID_ARG;
  tim->SR &= ~TIMx_SR_UIF;
  return HAL_OK;
}

/** @brief IRQ handler wrapper for TIM2. */
void TIM2_IRQHandler(void) {
  hal_timer_clear_interrupt_flag(TIM2);
  hal_interrupt_dispatch(TIM2_IRQn);
}

/** @brief IRQ handler wrapper for TIM3. */
void TIM3_IRQHandler(void) {
  hal_timer_clear_interrupt_flag(TIM3);
  hal_interrupt_dispatch(TIM3_IRQn);
}

/** @brief IRQ handler wrapper for TIM4. */
void TIM4_IRQHandler(void) {
  hal_timer_clear_interrupt_flag(TIM4);
  hal_interrupt_dispatch(TIM4_IRQn);
}

/** @brief IRQ handler wrapper for TIM5. */
void TIM5_IRQHandler(void) {
  hal_timer_clear_interrupt_flag(TIM5);
  hal_interrupt_dispatch(TIM5_IRQn);
}

/**
 * @brief IRQ handler for the shared TIM1-BRK / TIM9 vector.
 * @note Clears TIM9's flag and dispatches using the shared IRQn.
 */
void TIM1BRK_TIM9_IRQHandler(void) {
  hal_timer_clear_interrupt_flag(TIM9);
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
hal_status_t hal_timer_enable_interrupt(hal_timer_t timer) {
  switch (timer) {
  case TIM1:
    break; // [TODO] Implement the complex interrupt options
  case TIM2:
    hal_interrupt_enable(TIM2_IRQn);
    break;
  case TIM3:
    hal_interrupt_enable(TIM3_IRQn);
    break;
  case TIM4:
    hal_interrupt_enable(TIM4_IRQn);
    break;
  case TIM5:
    hal_interrupt_enable(TIM5_IRQn);
    break;
  case TIM9:
    hal_interrupt_enable(TIM1_BRK_TIM9_IRQn);
    break;
  default:
    break;
  }
  _set_interrupt_enable_bit(timer);
  return HAL_OK;
}

/**
 * @brief Disable a timer's update interrupt (NVIC + DIER UIE).
 * @param timer Timer identifier.
 * @return ::HAL_OK.
 */
hal_status_t hal_timer_disable_interrupt(hal_timer_t timer) {
  switch (timer) {
  case TIM1:
    break; // [TODO] Implement the complex interrupt options
  case TIM2:
    hal_interrupt_disable(TIM2_IRQn);
    break;
  case TIM3:
    hal_interrupt_disable(TIM3_IRQn);
    break;
  case TIM4:
    hal_interrupt_disable(TIM4_IRQn);
    break;
  case TIM5:
    hal_interrupt_disable(TIM5_IRQn);
    break;
  case TIM9:
    hal_interrupt_disable(TIM1_BRK_TIM9_IRQn);
    break;
  default:
    break;
  }
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim != NULL)
    tim->DIER &= ~TIMx_DIER_UIE;
  return HAL_OK;
}

/**
 * @brief Register a callback for a timer's update interrupt.
 * @param timer    Timer identifier.
 * @param callback Callback to invoke, or NULL to clear.
 * @return ::HAL_OK.
 */
hal_status_t hal_timer_attach_callback(hal_timer_t timer,
                                       hal_timer_callback_t callback) {
  switch (timer) {
  case TIM1:
    break; // [TODO] Implement the complex interrupt options
  case TIM2:
    hal_interrupt_attach_callback(TIM2_IRQn, callback);
    break;
  case TIM3:
    hal_interrupt_attach_callback(TIM3_IRQn, callback);
    break;
  case TIM4:
    hal_interrupt_attach_callback(TIM4_IRQn, callback);
    break;
  case TIM5:
    hal_interrupt_attach_callback(TIM5_IRQn, callback);
    break;
  case TIM9:
    hal_interrupt_attach_callback(TIM1_BRK_TIM9_IRQn, callback);
    break;
  default:
    break;
  }
  return HAL_OK;
}

/**
 * @brief Remove the callback registered for a timer's update interrupt.
 * @param timer Timer identifier.
 * @return ::HAL_OK.
 */
hal_status_t hal_timer_detach_callback(hal_timer_t timer) {
  switch (timer) {
  case TIM1:
    break; // [TODO] Implement the complex interrupt options
  case TIM2:
    hal_interrupt_detach_callback(TIM2_IRQn);
    break;
  case TIM3:
    hal_interrupt_detach_callback(TIM3_IRQn);
    break;
  case TIM4:
    hal_interrupt_detach_callback(TIM4_IRQn);
    break;
  case TIM5:
    hal_interrupt_detach_callback(TIM5_IRQn);
    break;
  case TIM9:
    hal_interrupt_detach_callback(TIM1_BRK_TIM9_IRQn);
    break;
  default:
    break;
  }
  return HAL_OK;
}

/**
 * @brief Set a channel's compare register and configure it for PWM mode 1.
 *
 * @param timer         Timer identifier.
 * @param channel       Channel number (1-4).
 * @param compare_value Value to write into CCRx.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an invalid timer/channel.
 */
hal_status_t hal_timer_set_compare(hal_timer_t timer, uint8_t channel,
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
  hal_timer_enable_channel(timer, channel);
  return HAL_OK;
}

/**
 * @brief Get a channel's compare register value.
 * @param timer   Timer identifier.
 * @param channel Channel number (1-4).
 * @return Compare value, or 0 for an invalid timer/channel.
 */
uint32_t hal_timer_get_compare(hal_timer_t timer, uint32_t channel) {
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
hal_status_t hal_timer_enable_channel(hal_timer_t timer, uint32_t channel) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL || channel < 1 || channel > 4)
    return HAL_ERR_INVALID_ARG;
  tim->CCER |= TIMx_CCER_CCxE_MASK(channel);
  if (timer == TIM1) {
    tim->BDTR |= TIMx_BDTR_MOE;
  }
  return HAL_OK;
}

/**
 * @brief Disable output on a timer channel (CCxE = 0).
 * @param timer   Timer identifier.
 * @param channel Channel number (1-4).
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an invalid timer/channel.
 */
hal_status_t hal_timer_disable_channel(hal_timer_t timer, uint32_t channel) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL || channel < 1 || channel > 4)
    return HAL_ERR_INVALID_ARG;
  tim->CCER &= (~TIMx_CCER_CCxE_MASK(channel));
  return HAL_OK;
}
