/**
 * @file timer.c
 * @brief Timer, SysTick and timer-peripheral helpers for STM32F4 (Cortex-M4).
 *
 * @details
 * This translation unit implements:
 * - SysTick initialization and a microsecond tick counter.
 * - Busy-wait delay helpers (`delay_us`, `delay_ms`).
 * - Timer peripheral base lookup, RCC enable, init/start/stop/reset.
 * - Timer interrupt control, attach/detach callbacks and IRQ handlers.
 * - Channel compare and PWM setup helpers.
 *
 * All logic is intentionally low-level (direct register access) and matches
 * the STM32F4 register layout / behavior expected by the rest of the HAL.
 */

#include "core/cortex-m4/timer.h"
#include "core/cortex-m4/clock.h"
#include "core/cortex-m4/interrupt.h"
#include "core/cortex-m4/uart.h"
#include "utils/timer_types.h"
#include <stdint.h>

/**
 * @brief Global system tick counter (increments in SysTick_Handler).
 *
 * @note Unit: ticks (tick duration set by systick_init).
 */
static volatile uint64_t systick_ticks = 0;     // global ticks

/**
 * @brief SysTick tick duration in microseconds.
 *
 * @note Default value is 1 us until systick_init is called.
 */
static volatile uint32_t tick_duration_us = 1;  // global tick duration in us

/**
 * @brief SysTick reload value (24-bit) currently configured.
 */
static volatile uint32_t tick_reload_value = 0; // ticks relaod value

/**
 * @brief Initialize the SysTick timer to generate periodic ticks.
 *
 * @param tick_us Tick period in microseconds.
 *
 * @note The SysTick reload is limited to 24 bits; this function clips the
 *       computed reload value to 24 bits. The function configures SysTick
 *       to use the selected clock source, enables the SysTick interrupt
 *       and starts the timer.
 */
void systick_init(uint32_t tick_us) {
  // systick interrupt is not under the NVIC
  tick_duration_us = tick_us;
  uint32_t ahbclk = hal_clock_get_ahbclk();
  uint64_t reload_value = (((uint64_t)ahbclk * tick_us) / 1000000ULL) - 1;
  reload_value &= 0xffffff; // set the reload value as 24bit only
  uint32_t reload = (uint32_t)reload_value;
  tick_reload_value = reload;
  SYST_RVR = reload; // Set reload value
  SYST_CVR = 0;      // Clear current value
  SYST_CSR =
      (1 << SYST_CSR_EN_BIT) | (1 << SYST_CSR_TICKINT_BIT) |
      (1 << SYST_CSR_CLKSOURCE_BIT); // Enable | TickInt | ClkSource
                                     // CLOCK source 0 means the systick run at
                                     // ahbclk/8 and if it's 1 it runs at ahbclk
}

/**
 * @brief Busy-wait for the specified number of microseconds.
 *
 * @param us Number of microseconds to delay.
 *
 * @note This is a blocking busy-wait that uses hal_get_tick() and the
 *       configured tick duration. It will wait at least one tick if
 *       the requested delay is smaller than the tick duration.
 */
void delay_us(uint64_t us) {
  volatile uint64_t ticks_needed = us / hal_get_tick_duration_us();
  if (ticks_needed ==
      0) // if 0 ticks are needed for the delay raise it to 1 tick
    ticks_needed = 1;
  volatile uint32_t start = hal_get_tick();
  while (hal_get_tick() - start < ticks_needed)
    __asm__ volatile("nop"); // insert noops in bw
  ;
}

/**
 * @brief Busy-wait for the specified number of milliseconds.
 *
 * @param ms Number of milliseconds to delay.
 */
void delay_ms(uint32_t ms) { delay_us(ms * 1000); }

/**
 * @brief Return the current system tick count.
 *
 * @return Current tick count.
 */
uint64_t hal_get_tick(void) { return systick_ticks; }

/**
 * @brief Return the configured tick duration in microseconds.
 *
 * @return Tick duration (us).
 */
uint32_t hal_get_tick_duration_us(void) { return tick_duration_us; }

/**
 * @brief Return the SysTick reload value (24-bit truncated).
 *
 * @return Reload value currently configured in SYST_RVR.
 */
uint32_t hal_get_tick_reload_value(void) { return tick_reload_value; }

/**
 * @brief Return system uptime in milliseconds.
 *
 * @return Milliseconds since tick counter started.
 */
uint32_t hal_get_millis(void) { return hal_get_micros() / 1000; }

/**
 * @brief Return system uptime in microseconds.
 *
 * @return Microseconds since tick counter started.
 */
uint32_t hal_get_micros(void) {
  return hal_get_tick() * hal_get_tick_duration_us();
}

/**
 * @brief SysTick interrupt handler increments the global tick counter.
 *
 * @note This handler is intended to be wired into the vector table.
 */
void SysTick_Handler(void) { systick_ticks++; }

/**
 * @internal
 * @brief Map hal_timer_t to the timer peripheral base address.
 *
 * @param timer Timer identifier (hal_timer_t).
 * @return Base address of the timer peripheral or 0 if invalid.
 */
uint32_t _get_timer_base(hal_timer_t timer) {
  switch (timer) {
  case TIM1:
    return TIM1_BASE;
  case TIM2:
    return TIM2_BASE;
  case TIM3:
    return TIM3_BASE;
  case TIM4:
    return TIM4_BASE;
  case TIM5:
    return TIM5_BASE;
  case TIM9:
    return TIM9_BASE;
  case TIM10:
    return TIM10_BASE;
  case TIM11:
    return TIM11_BASE;
  default:
    return 0;
  }
}

/**
 * @internal
 * @brief Enable peripheral clock for the given timer in RCC registers.
 *
 * @param timer Timer identifier (hal_timer_t).
 */
void _enable_timer_rcc(hal_timer_t timer) {
  switch (timer) {
  case TIM1:
    RCC_APB2ENR |= (1 << RCC_APB2ENR_TIM1_OFFSET);
    break;
  case TIM2:
    RCC_APB1ENR |= (1 << RCC_APB1ENR_TIM2_OFFSET);
    break;
  case TIM3:
    RCC_APB1ENR |= (1 << RCC_APB1ENR_TIM3_OFFSET);
    break;
  case TIM4:
    RCC_APB1ENR |= (1 << RCC_APB1ENR_TIM4_OFFSET);
    break;
  case TIM5:
    RCC_APB1ENR |= (1 << RCC_APB1ENR_TIM5_OFFSET);
    break;
  case TIM9:
    RCC_APB2ENR |= (1 << RCC_APB2ENR_TIM9_OFFSET);
    break;
  case TIM10:
    RCC_APB2ENR |= (1 << RCC_APB2ENR_TIM10_OFFSET);
    break;
  case TIM11:
    RCC_APB2ENR |= (1 << RCC_APB2ENR_TIM11_OFFSET);
    break;
  default:
    break;
  }
}

/**
 * @internal
 * @brief Initialize a General Purpose timer on APB1 (TIM2-TIM5).
 *
 * @param timer Timer identifier.
 * @param prescaler Prescaler value to write to PSC register.
 * @param auto_reload Auto-reload (ARR) value.
 */
void _timer_gp1_init(hal_timer_t timer, uint32_t prescaler,
                     uint32_t auto_reload) {
  uint32_t timer_base = _get_timer_base(timer);
  if (timer_base == 0)
    return;
  // Enable clock
  _enable_timer_rcc(timer);

  volatile uint32_t *timx_cr1 =
      (volatile uint32_t *)(timer_base + TIM_GP1_CR1_OFFSET);

  // Disable the timer (if active)
  (*timx_cr1) = (*timx_cr1) & (~(1 << TIM_GP1_CR1_CEN_BIT));

  volatile uint32_t *timx_psc =
      (volatile uint32_t *)(timer_base + TIM_GP1_PSC_OFFSET);
  (*timx_psc) = prescaler;

  // set counter to 0
  volatile uint32_t *timx_cnt =
      (volatile uint32_t *)(timer_base + TIM_GP1_CNT_OFFSET);
  (*timx_cnt) = 0;

  // remove upper 16 bits
  if (!(timer == TIM2 || timer == TIM5))
    auto_reload = (uint16_t)auto_reload;

  volatile uint32_t *timx_arr =
      (volatile uint32_t *)(timer_base + TIM_GP1_ARR_OFFSET);
  (*timx_arr) = auto_reload;

  volatile uint32_t *timx_egr =
      (volatile uint32_t *)(timer_base + TIM_GP1_EGR_OFFSET);
  (*timx_egr) = (1 << TIM_GP1_EGR_UG_BIT);

  (*timx_cr1) = (1 << TIM_GP1_CR1_CEN_BIT);
}

/**
 * @internal
 * @brief Initialize a General Purpose timer on APB2 (TIM9-TIM11).
 *
 * @param timer Timer identifier.
 * @param prescaler Prescaler value to write to PSC register (16-bit).
 * @param auto_reload Auto-reload (ARR) value (16-bit).
 */
void _timer_gp2_init(hal_timer_t timer, uint32_t prescaler,
                     uint32_t auto_reload) {
  uint32_t timer_base = _get_timer_base(timer);
  if (timer_base == 0)
    return;
  // Enable clock
  _enable_timer_rcc(timer);

  volatile uint32_t *timx_cr1 =
      (volatile uint32_t *)(timer_base + TIM_GP2_CR1_OFFSET);

  // Disable the timer (if active)
  (*timx_cr1) = (*timx_cr1) & (~(1 << TIM_GP2_CR1_CEN_BIT));

  volatile uint16_t *timx_psc =
      (volatile uint16_t *)(timer_base + TIM_GP2_PSC_OFFSET);
  (*timx_psc) = prescaler;

  // set counter to 0
  volatile uint16_t *timx_cnt =
      (volatile uint16_t *)(timer_base + TIM_GP2_CNT_OFFSET);
  (*timx_cnt) = 0;

  // remove upper 16 bits
  auto_reload = (uint16_t)auto_reload;

  volatile uint16_t *timx_arr =
      (volatile uint16_t *)(timer_base + TIM_GP2_ARR_OFFSET);
  (*timx_arr) = auto_reload;

  volatile uint32_t *timx_egr =
      (volatile uint32_t *)(timer_base + TIM_GP2_EGR_OFFSET);
  (*timx_egr) = (1 << TIM_GP2_EGR_UG_BIT);

  (*timx_cr1) = (1 << TIM_GP2_CR1_CEN_BIT);
}

/**
 * @internal
 * @brief Initialize an Advanced timer (TIM1).
 *
 * @param timer Timer identifier.
 * @param prescaler Prescaler value to write to PSC register.
 * @param auto_reload Auto-reload (ARR) value.
 */
void _timer_adv_init(hal_timer_t timer, uint32_t prescaler,
                     uint32_t auto_reload) {
  uint32_t timer_base = _get_timer_base(timer);
  if (timer_base == 0)
    return;
  // Enable clock
  _enable_timer_rcc(timer);

  volatile uint32_t *timx_cr1 =
      (volatile uint32_t *)(timer_base + TIM_ADV_CR1_OFFSET);

  // Disable the timer (if active)
  (*timx_cr1) = (*timx_cr1) & (~(1 << TIM_ADV_CR1_CEN_BIT));

  volatile uint32_t *timx_psc =
      (volatile uint32_t *)(timer_base + TIM_ADV_PSC_OFFSET);
  (*timx_psc) = prescaler;

  // set counter to 0
  volatile uint32_t *timx_cnt =
      (volatile uint32_t *)(timer_base + TIM_ADV_CNT_OFFSET);
  (*timx_cnt) = 0;

  // remove upper 16 bits
  if (!(timer == TIM2 || timer == TIM5))
    auto_reload = (uint16_t)auto_reload;

  volatile uint32_t *timx_arr =
      (volatile uint32_t *)(timer_base + TIM_ADV_ARR_OFFSET);
  (*timx_arr) = auto_reload;

  volatile uint32_t *timx_egr =
      (volatile uint32_t *)(timer_base + TIM_ADV_EGR_OFFSET);
  (*timx_egr) = (1 << TIM_ADV_EGR_UG_BIT);

  (*timx_cr1) = (1 << TIM_ADV_CR1_CEN_BIT);
}

/**
 * @brief Initialize a timer based on its type (advanced / gp1 / gp2).
 *
 * @param timer Timer identifier.
 * @param prescaler Prescaler value.
 * @param auto_reload Auto-reload value.
 */
void timer_init(hal_timer_t timer, uint32_t prescaler, uint32_t auto_reload) {
  if (timer == TIM1)
    _timer_adv_init(timer, prescaler, auto_reload);
  else if (timer >= TIM2 && timer <= TIM5)
    _timer_gp1_init(timer, prescaler, auto_reload);
  else if (timer >= TIM9 && timer <= TIM11)
    _timer_gp2_init(timer, prescaler, auto_reload);
}

/**
 * @brief Start the specified timer.
 *
 * @param timer Timer identifier.
 */
void timer_start(hal_timer_t timer) {
  uint32_t timer_base = _get_timer_base(timer);
  if (timer == TIM1) {
    volatile uint32_t *timer_cr1 =
        (volatile uint32_t *)(timer_base + TIM_ADV_CR1_OFFSET);
    (*timer_cr1) = (*timer_cr1) | (1 << TIM_ADV_CR1_CEN_BIT);
  } else if (timer >= TIM2 && timer <= TIM5) {
    volatile uint32_t *timer_cr1 =
        (volatile uint32_t *)(timer_base + TIM_GP1_CR1_OFFSET);
    (*timer_cr1) = (*timer_cr1) | (1 << TIM_GP1_CR1_CEN_BIT);
  } else if (timer >= TIM9 && timer <= TIM11) {
    volatile uint32_t *timer_cr1 =
        (volatile uint32_t *)(timer_base + TIM_GP2_CR1_OFFSET);
    (*timer_cr1) = (*timer_cr1) | (1 << TIM_GP2_CR1_CEN_BIT);
  }
}

/**
 * @brief Stop the specified timer.
 *
 * @param timer Timer identifier.
 */
void timer_stop(hal_timer_t timer) {
  uint32_t timer_base = _get_timer_base(timer);
  if (timer == TIM1) {
    volatile uint32_t *timer_cr1 =
        (volatile uint32_t *)(timer_base + TIM_ADV_CR1_OFFSET);
    (*timer_cr1) = (*timer_cr1) & (~(1 << TIM_ADV_CR1_CEN_BIT));
  } else if (timer >= TIM2 && timer <= TIM5) {
    volatile uint32_t *timer_cr1 =
        (volatile uint32_t *)(timer_base + TIM_GP1_CR1_OFFSET);
    (*timer_cr1) = (*timer_cr1) & (~(1 << TIM_GP1_CR1_CEN_BIT));
  } else if (timer >= TIM9 && timer <= TIM11) {
    volatile uint32_t *timer_cr1 =
        (volatile uint32_t *)(timer_base + TIM_GP2_CR1_OFFSET);
    (*timer_cr1) = (*timer_cr1) & (~(1 << TIM_GP2_CR1_CEN_BIT));
  }
}

/**
 * @brief Reset timer counter to zero.
 *
 * @param timer Timer identifier.
 */
void timer_reset(hal_timer_t timer) {
  uint32_t timer_base = _get_timer_base(timer);

  if (timer == TIM1) {
    *((volatile uint32_t *)(timer_base + TIM_ADV_CNT_OFFSET)) = 0;
  } else if (timer >= TIM2 && timer <= TIM5) {
    *((volatile uint32_t *)(timer_base + TIM_GP1_CNT_OFFSET)) = 0;
  } else if (timer >= TIM9 && timer <= TIM11) {
    *((volatile uint16_t *)(timer_base + TIM_GP2_CNT_OFFSET)) = 0;
  }
}

/**
 * @brief Get the current counter value for a timer.
 *
 * @param timer Timer identifier.
 * @return Current counter value or 0 if invalid timer.
 */
uint32_t timer_get_count(hal_timer_t timer) {
  uint32_t timer_base = _get_timer_base(timer);
  if (timer == TIM1) {
    return *((volatile uint32_t *)(timer_base + TIM_ADV_CNT_OFFSET));
  } else if (timer >= TIM2 && timer <= TIM5) {
    return *((volatile uint32_t *)(timer_base + TIM_GP1_CNT_OFFSET));
  } else if (timer >= TIM9 && timer <= TIM11) {
    return *((volatile uint32_t *)(timer_base + TIM_GP2_CNT_OFFSET));
  }
  return 0;
}

/**
 * @brief Calculate the timer's current base frequency using PSC and ARR.
 *
 * @param timer Timer identifier.
 * @return Timer frequency in Hz or 0 if invalid timer.
 *
 * @note This reads PSC and ARR directly from timer registers and uses
 *       the appropriate APB clock for the timer.
 */
uint32_t timer_get_frequency(hal_timer_t timer) {
  uint32_t timer_base = _get_timer_base(timer);
  if (timer_base == 0)
    return 0;

  // 1. Get clock
  uint32_t timer_clk = hal_clock_get_apb1clk(); // default for TIM2-TIM5
  if (timer == TIM1 || timer == TIM9 || timer == TIM10 || timer == TIM11) {
    timer_clk = hal_clock_get_apb2clk(); // For advanced timers
  }

  // 2. Get prescaler and ARR
  uint32_t prescaler = 0;
  uint32_t arr = 0;

  if (timer == TIM1) {
    prescaler = *((volatile uint32_t *)(timer_base + TIM_ADV_PSC_OFFSET));
    arr = *((volatile uint32_t *)(timer_base + TIM_ADV_ARR_OFFSET));
  } else if (timer >= TIM2 && timer <= TIM5) {
    prescaler = *((volatile uint32_t *)(timer_base + TIM_GP1_PSC_OFFSET));
    arr = *((volatile uint32_t *)(timer_base + TIM_GP1_ARR_OFFSET));
  } else if (timer >= TIM9 && timer <= TIM11) {
    prescaler = *((volatile uint16_t *)(timer_base + TIM_GP2_PSC_OFFSET));
    arr = *((volatile uint16_t *)(timer_base + TIM_GP2_ARR_OFFSET));
  }

  if (arr == 0x0)
    arr = 0xFFFF; // Prevent division by zero

  // 3. Calculate frequency
  uint32_t freq = timer_clk / (prescaler + 1) / (arr + 1);
  return freq;
}
void timer_set_arr(hal_timer_t timer, uint32_t channel, uint32_t arr){

}

void timer_clear_interrupt_flag(hal_timer_t timer) {
  // for tim2-5  & 9only
  uint32_t timer_base = _get_timer_base(timer);
  volatile uint32_t *timer_sr =
      (volatile uint32_t *)(timer_base + TIM_GP1_SR_OFFSET);
  if (timer >= TIM2 && timer <= TIM5) {
    (*timer_sr) &= (~(1 << TIM_GP1_SR_UIF_BIT));
  } else if (timer >= TIM9 && timer <= TIM11)
    (*timer_sr) &= (~(1 << TIM_GP2_SR_UIF_BIT));
}

/**
 * @brief IRQ handler wrapper for TIM2.
 *
 * Clears the flag and dispatches to the HAL interrupt handler table.
 */
void TIM2_IRQHandler() {
  timer_clear_interrupt_flag(TIM2);
  hal_handle_interrupt(TIM2_IRQn);
}

/**
 * @brief IRQ handler wrapper for TIM3.
 */
void TIM3_IRQHandler() {
  timer_clear_interrupt_flag(TIM3);
  hal_handle_interrupt(TIM3_IRQn);
}

/**
 * @brief IRQ handler wrapper for TIM4.
 */
void TIM4_IRQHandler() {
  timer_clear_interrupt_flag(TIM4);
  hal_handle_interrupt(TIM4_IRQn);
}

/**
 * @brief IRQ handler wrapper for TIM5.
 */
void TIM5_IRQHandler() {
  timer_clear_interrupt_flag(TIM5);
  hal_handle_interrupt(TIM5_IRQn);
}

/**
 * @brief IRQ handler for TIM1 BRK and TIM9 shared vector.
 *
 * @note This function clears TIM9's flag and dispatches using the shared IRQn.
 */
void TIM1BRK_TIM9_IRQHandler() {
  timer_clear_interrupt_flag(TIM9);
  hal_handle_interrupt(TIM1_BRK_TIM9_IRQn); // shared with TIM1 BRK
}

/**
 * @internal
 * @brief Set the update interrupt enable bit in DIER for the specified timer.
 *
 * @param timer Timer identifier.
 */
void _set_interrupt_enable_bit(hal_timer_t timer) {
  uint32_t timer_base = _get_timer_base(timer);
  if (timer == TIM1) {
    volatile uint32_t *timer_dier =
        (volatile uint32_t *)(timer_base + TIM_ADV_DIER_OFFSET);
    (*timer_dier) = (*timer_dier) | (1 << TIM_ADV_DIER_UIE_BIT);
  } else if (timer >= TIM2 && timer <= TIM5) {
    volatile uint32_t *timer_dier =
        (volatile uint32_t *)(timer_base + TIM_GP1_DIER_OFFSET);
    (*timer_dier) = (*timer_dier) | (1 << TIM_GP1_DIER_UIE_BIT);
  } else if (timer >= TIM9 && timer <= TIM11) {
    volatile uint32_t *timer_dier =
        (volatile uint32_t *)(timer_base + TIM_GP2_DIER_OFFSET);
    (*timer_dier) = (*timer_dier) | (1 << TIM_GP2_DIER_UIE_BIT);
  }
}

/**
 * @brief Enable timer interrupts (NVIC + DIER UIE) for supported timers.
 *
 * @param timer Timer identifier.
 *
 * @note For TIM1 more complex options exist and are left TODO in the original.
 */
void timer_enable_interrupt(hal_timer_t timer) {
  // [TODO]: add all timer interrupts
  switch (timer) {
  case TIM1:
    break; // [TODO] Implement the complex interrupt options
  case TIM2:
    hal_enable_interrupt(TIM2_IRQn);
    break;
  case TIM3:
    hal_enable_interrupt(TIM3_IRQn);
    break;
  case TIM4:
    hal_enable_interrupt(TIM4_IRQn);
    break;
  case TIM5:
    hal_enable_interrupt(TIM5_IRQn);
    break;
  case TIM9:
    hal_enable_interrupt(TIM1_BRK_TIM9_IRQn);
    break;
  default:
    break;
  }
  _set_interrupt_enable_bit(timer);
}

/**
 * @brief Attach a callback to the timer's HAL interrupt table.
 *
 * @param timer Timer identifier.
 * @param callback Function pointer to call when the timer IRQ fires.
 */
void timer_attach_callback(hal_timer_t timer, void (*callback)(void)) {
  // [TODO]: add all timer interrupts
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
}

/**
 * @brief Detach a previously attached callback for the given timer.
 *
 * @param timer Timer identifier.
 */
void timer_detach_callback(hal_timer_t timer) {
  // [TODO]: add all timer interrupts
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
}

/**
 * @brief Set the compare (CCR) register for a timer channel and configure CCMR.
 *
 * @param timer Timer identifier.
 * @param channel Channel number (1-4).
 * @param compare_value Value to write into CCRx.
 *
 * @note This function sets PWM mode 1 and enables the channel after writing CCR.
 */
void timer_set_compare(hal_timer_t timer, uint8_t channel,
                       uint32_t compare_value) {
  uint32_t timer_base = _get_timer_base(timer);
  if (channel < 1 || channel > 4)
    return; // only 4 valid channels
  volatile uint32_t *ccr_reg =
      (volatile uint32_t *)(timer_base + TIMx_CCR1_OFFSET + (channel - 1) * 4);
  *ccr_reg = compare_value;
  volatile uint32_t *ccmr_reg;
  if (channel <= 2)
    ccmr_reg = (volatile uint32_t *)(timer_base + TIMx_CCMR1_OFFSET);
  else
    ccmr_reg = (volatile uint32_t *)(timer_base + TIMx_CCMR2_OFFSET);

  (*ccmr_reg) &= (0x7 << TIMx_CCMR1_OC1M_BIT);
  (*ccmr_reg) |= (TIMx_CCMR1_OC1M_PWM_MODE1 << TIMx_CCMR1_OC1M_BIT);
  (*ccmr_reg) |= (1 << TIMx_CCMR1_OC1PE_BIT);
  timer_enable_channel(timer, channel);
}
uint32_t timer_get_compare(hal_timer_t timer, uint32_t channel) {

  uint32_t timer_base = _get_timer_base(timer);
  if (channel < 1 || channel > 4)
    return 0; // only 4 valid channels
  return *(volatile uint32_t *)(timer_base + TIMx_CCR1_OFFSET +
                                (channel - 1) * 4);
}
uint32_t timer_get_arr(hal_timer_t timer, uint32_t channel) {
  uint32_t timer_base = _get_timer_base(timer);
  if (timer_base == 0)
    return 0;
  volatile uint32_t arr = 0;
  if (timer == TIM1) {
    arr = *((volatile uint32_t *)(timer_base + TIM_ADV_ARR_OFFSET));
  } else if (timer >= TIM2 && timer <= TIM5) {
    arr = *((volatile uint32_t *)(timer_base + TIM_GP1_ARR_OFFSET));
  } else if (timer >= TIM9 && timer <= TIM11) {
    arr = *((volatile uint16_t *)(timer_base + TIM_GP2_ARR_OFFSET));
  }
  return arr;
}
void timer_enable_channel(hal_timer_t timer, uint32_t channel) {

  uint32_t timer_base = _get_timer_base(timer);
  volatile uint32_t *ccer_reg =
      (volatile uint32_t *)(timer_base + TIMx_CCER_OFFSET);

  if (channel < 1 || channel > 4)
    return; // only 4 valid channels
  switch (channel) {
  case 1:
    *ccer_reg |= (1 << TIMx_CCER_CC1E_BIT);
    break;
  case 2:
    *ccer_reg |= (1 << TIMx_CCER_CC2E_BIT);
    break;
  case 3:
    *ccer_reg |= (1 << TIMx_CCER_CC3E_BIT);
    break;
  case 4:
    *ccer_reg |= (1 << TIMx_CCER_CC4E_BIT);
    break;
  default:
    break;
  }
}

/**
 * @brief Disable output on the specified timer channel (CCxE = 0).
 *
 * @param timer Timer identifier.
 * @param channel Channel number (1-4).
 */
void timer_disable_channel(hal_timer_t timer, uint32_t channel) {

  uint32_t timer_base = _get_timer_base(timer);
  volatile uint32_t *ccer_reg =
      (volatile uint32_t *)(timer_base + TIMx_CCER_OFFSET);

  if (channel < 1 || channel > 4)
    return; // only 4 valid channels
  switch (channel) {
  case 1:
    *ccer_reg &= ~(1 << TIMx_CCER_CC1E_BIT);
    break;
  case 2:
    *ccer_reg &= ~(1 << TIMx_CCER_CC2E_BIT);
    break;
  case 3:
    *ccer_reg &= ~(1 << TIMx_CCER_CC3E_BIT);
    break;
  case 4:
    *ccer_reg &= ~(1 << TIMx_CCER_CC4E_BIT);
    break;
  default:
    break;
  }
}
