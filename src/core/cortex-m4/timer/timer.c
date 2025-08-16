/**
 * @file timer.c
 * @brief Timer, SysTick and timer-peripheral helpers for STM32F4 (Cortex-M4)
 * @details
 * This translation unit implements:
 * - SysTick initialization and microsecond tick counter
 * - Busy-wait delay helpers (delay_us, delay_ms)
 * - Timer peripheral base lookup, RCC enable, init/start/stop/reset
 * - Timer interrupt control, attach/detach callbacks and IRQ handlers
 * - Channel compare and PWM setup helpers
 *
 * All logic uses direct register access matching the STM32F4 register layout.
 * 
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "core/cortex-m4/timer.h"
#include "core/cortex-m4/clock.h"
#include "core/cortex-m4/interrupt.h"
#include "core/cortex-m4/rcc_reg.h"
#include "core/cortex-m4/timer_reg.h"
#include "utils/timer_types.h"
#include <stdint.h>

<<<<<<< HEAD
/* Global variables */
static volatile uint64_t systick_ticks = 0;     ///< Global system tick counter
static volatile uint32_t tick_duration_us = 1;  ///< SysTick tick duration in us
static volatile uint32_t tick_reload_value = 0; ///< Current SysTick reload value
=======
/**
 * @brief Global system tick counter (increments in SysTick_Handler).
 *
 * @note Unit: ticks (tick duration set by systick_init).
 */
static volatile uint64_t systick_ticks = 0; // global ticks

/**
 * @brief SysTick tick duration in microseconds.
 *
 * @note Default value is 1 us until systick_init is called.
 */
static volatile uint32_t tick_duration_us = 1; // global tick duration in us

/**
 * @brief SysTick reload value (24-bit) currently configured.
 */
static volatile uint32_t tick_reload_value = 0; // ticks relaod value
>>>>>>> 73eb6c283269d3698b418665a84bf3716bb15e61

/**
 * @brief Initialize the SysTick timer
 * @param tick_us Tick period in microseconds
 * @details
 * Configures SysTick to:
 * - Use processor clock (AHB)
 * - Generate interrupts at specified interval
 * - Start counting immediately
 * @note Reload value is limited to 24 bits
 */
void systick_init(uint32_t tick_us) {
  tick_duration_us = tick_us;
  uint32_t ahbclk = hal_clock_get_ahbclk();
  uint64_t reload_value = (((uint64_t)ahbclk * tick_us) / 1000000ULL) - 1;
  reload_value &= 0xffffff; // 24-bit reload
  uint32_t reload = (uint32_t)reload_value;
  tick_reload_value = reload;
  SYST_RVR = reload; // Set reload value
  SYST_CVR = 0;      // Clear current value
  SYST_CSR = (1 << SYST_CSR_EN_BIT) | (1 << SYST_CSR_TICKINT_BIT) |
              (1 << SYST_CSR_CLKSOURCE_BIT); // Enable | TickInt | ClkSource
}

/**
 * @brief Busy-wait delay in microseconds
 * @param us Number of microseconds to delay
 * @note Minimum delay is one tick period
 */
void delay_us(uint64_t us) {
  volatile uint64_t ticks_needed = us / hal_get_tick_duration_us();
  if (ticks_needed == 0) ticks_needed = 1;
  volatile uint32_t start = hal_get_tick();
  while (hal_get_tick() - start < ticks_needed)
    __asm__ volatile("nop");
}

/**
 * @brief Busy-wait delay in milliseconds 
 * @param ms Number of milliseconds to delay
 */
void delay_ms(uint32_t ms) { delay_us(ms * 1000); }

/**
 * @brief Get current system tick count
 * @return Current tick count
 */
uint64_t hal_get_tick(void) { return systick_ticks; }

/**
 * @brief Get configured tick duration
 * @return Tick duration in microseconds
 */
uint32_t hal_get_tick_duration_us(void) { return tick_duration_us; }

/**
 * @brief Get SysTick reload value
 * @return Current reload value (24-bit)
 */
uint32_t hal_get_tick_reload_value(void) { return tick_reload_value; }

/**
 * @brief Get system uptime in milliseconds
 * @return Milliseconds since startup
 */
uint32_t hal_get_millis(void) { return hal_get_micros() / 1000; }

/**
 * @brief Get system uptime in microseconds
 * @return Microseconds since startup
 */
uint32_t hal_get_micros(void) {
  return hal_get_tick() * hal_get_tick_duration_us();
}

/**
 * @brief SysTick interrupt handler
 * @details Increments the global tick counter
 */
void SysTick_Handler(void) { systick_ticks++; }

/* Timer peripheral helper functions */

/**
<<<<<<< HEAD
 * @brief Get timer base address
 * @param timer Timer identifier
 * @return Base address or 0 if invalid
 * @note Internal helper function
 */
static uint32_t _get_timer_base(hal_timer_t timer) {
  switch (timer) {
  case TIM1: return TIM1_BASE;
  case TIM2: return TIM2_BASE;
  /* ... other timers ... */
  default: return 0;
  }
}

/**
 * @brief Enable timer peripheral clock
 * @param timer Timer identifier
 * @note Internal helper function
=======
 * @internal
 * @brief Enable peripheral clock for the given timer in RCC registers.
 *
 * @param timer Timer identifier (hal_timer_t).
>>>>>>> 73eb6c283269d3698b418665a84bf3716bb15e61
 */
static void _enable_timer_rcc(hal_timer_t timer) {
  switch (timer) {
<<<<<<< HEAD
  case TIM1: RCC_APB2ENR |= (1 << RCC_APB2ENR_TIM1_OFFSET); break;
  /* ... other timers ... */
=======
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
>>>>>>> 73eb6c283269d3698b418665a84bf3716bb15e61
  }
}

/**
<<<<<<< HEAD
 * @brief Initialize general purpose timer (APB1)
 * @param timer Timer identifier
 * @param prescaler Prescaler value
 * @param auto_reload Auto-reload value
 * @note Internal helper function
 */
static void _timer_gp1_init(hal_timer_t timer, uint32_t prescaler,
                     uint32_t auto_reload) {
  /* ... implementation ... */
}

/**
 * @brief Initialize timer based on type
 * @param timer Timer identifier
 * @param prescaler Prescaler value
 * @param auto_reload Auto-reload value
 */
void timer_init(hal_timer_t timer, uint32_t prescaler, uint32_t auto_reload) {
  if (timer == TIM1)
    _timer_adv_init(timer, prescaler, auto_reload);
  else if (timer >= TIM2 && timer <= TIM5)
    _timer_gp1_init(timer, prescaler, auto_reload);
  else if (timer >= TIM9 && timer <= TIM11)
    _timer_gp2_init(timer, prescaler, auto_reload);
=======
 * @brief Initialize a timer based on its type (advanced / gp1 / gp2).
 *
 * @param timer Timer identifier.
 * @param prescaler Prescaler value.
 * @param auto_reload Auto-reload value.
 */
void timer_init(hal_timer_t timer, uint32_t prescaler, uint32_t auto_reload) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return;
  // Enable clock
  _enable_timer_rcc(timer);
  tim->CR1 &= (~(TIMx_CR1_CEN));
  tim->PSC = prescaler;
  tim->CNT = 0;
  if (!(timer == TIM2 || timer == TIM5))
    auto_reload = (uint16_t)auto_reload;
  tim->ARR = auto_reload;
  tim->EGR |= TIMx_EGR_UG;
  tim->CR1 |= TIMx_CR1_CEN;
>>>>>>> 73eb6c283269d3698b418665a84bf3716bb15e61
}

/**
 * @brief Start timer counter
 * @param timer Timer identifier
 */
void timer_start(hal_timer_t timer) {
<<<<<<< HEAD
  /* ... implementation ... */
=======
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return;
  tim->CR1 |= TIMx_CR1_CEN;
>>>>>>> 73eb6c283269d3698b418665a84bf3716bb15e61
}

/**
 * @brief Stop timer counter
 * @param timer Timer identifier
 */
void timer_stop(hal_timer_t timer) {
<<<<<<< HEAD
  /* ... implementation ... */
}

/* ... remaining timer functions with similar documentation ... */

/**
 * @brief Set timer compare value for PWM
 * @param timer Timer identifier
 * @param channel Channel number (1-4)
 * @param compare_value CCR register value
 * @details
 * Configures PWM mode 1 and enables the channel output
 */
void timer_set_compare(hal_timer_t timer, uint8_t channel,
                       uint32_t compare_value) {
  /* ... implementation ... */
}

/**
 * @brief Enable timer channel output
 * @param timer Timer identifier
 * @param channel Channel number (1-4)
 */
void timer_enable_channel(hal_timer_t timer, uint32_t channel) {
  /* ... implementation ... */
=======
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return;
  tim->CR1 &= (~TIMx_CR1_CEN);
}

/**
 * @brief Reset timer counter to zero.
 *
 * @param timer Timer identifier.
 */
void timer_reset(hal_timer_t timer) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return;
  tim->CNT = 0;
}

/**
 * @brief Get the current counter value for a timer.
 *
 * @param timer Timer identifier.
 * @return Current counter value or 0 if invalid timer.
 */
uint32_t timer_get_count(hal_timer_t timer) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return 0;
  return tim->CNT;
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
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return 0;

  // 1. Get clock
  uint32_t timer_clk = hal_clock_get_apb1clk(); // default for TIM2-TIM5
  if (timer == TIM1 || timer == TIM9 || timer == TIM10 || timer == TIM11) {
    timer_clk = hal_clock_get_apb2clk(); // For advanced timers
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
void timer_set_arr(hal_timer_t timer, uint32_t channel, uint32_t arr) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return;

  timer_stop(timer);
  if (!(timer == TIM2 || timer == TIM5))
    arr = (uint16_t)arr;
  tim->ARR = arr;
  timer_start(timer);
}

void timer_clear_interrupt_flag(hal_timer_t timer) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return;
  tim->SR |= TIMx_SR_UIF;
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
 * @note This function clears TIM9's flag and dispatches using the shared
 * IRQn.
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
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return;
  tim->DIER |= TIMx_DIER_UIE;
}

/**
 * @brief Enable timer interrupts (NVIC + DIER UIE) for supported timers.
 *
 * @param timer Timer identifier.
 *
 * @note For TIM1 more complex options exist and are left TODO in the
 * original.
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
 * @brief Set the compare (CCR) register for a timer channel and configure
 * CCMR.
 *
 * @param timer Timer identifier.
 * @param channel Channel number (1-4).
 * @param compare_value Value to write into CCRx.
 *
 * @note This function sets PWM mode 1 and enables the channel after writing
 * CCR.
 */
void timer_set_compare(hal_timer_t timer, uint8_t channel,
                       uint32_t compare_value) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL)
    return;
  if (channel < 1 || channel > 4)
    return; // only 4 valid channels
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
  if (channel <= 2) {
    tim->CCMR1 &= (~TIMx_CCMRy_OCzM_MASK(channel));
    tim->CCMR1 |= (~TIMx_CCMRy_OCzM_PWM_MODE1_MASK(channel));
    tim->CCMR1 |= TIMx_CCMRy_OCxPE(channel);
  } else {
    tim->CCMR2 &= (~TIMx_CCMRy_OCzM_MASK(channel));
    tim->CCMR2 |= (~TIMx_CCMRy_OCzM_PWM_MODE1_MASK(channel));
    tim->CCMR2 |= TIMx_CCMRy_OCxPE(channel);
  }
  timer_enable_channel(timer, channel);
}
uint32_t timer_get_compare(hal_timer_t timer, uint32_t channel) {
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

uint32_t timer_get_arr(hal_timer_t timer, uint32_t channel) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL || channel < 1 || channel > 4)
    return 0;
  return tim->ARR;
}

void timer_enable_channel(hal_timer_t timer, uint32_t channel) {
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL || channel < 1 || channel > 4)
    return;
  tim->CCER |= TIMx_CCER_CCxE_MASK(channel);
>>>>>>> 73eb6c283269d3698b418665a84bf3716bb15e61
}

/**
 * @brief Disable timer channel output
 * @param timer Timer identifier
 * @param channel Channel number (1-4)
 */
void timer_disable_channel(hal_timer_t timer, uint32_t channel) {
<<<<<<< HEAD
  /* ... implementation ... */
}
=======
  TIMx_Reg_Typedef *tim = GET_TIMx_BASE(timer);
  if (tim == NULL || channel < 1 || channel > 4)
    return;
  tim->CCER &= (~TIMx_CCER_CCxE_MASK(channel));
}
>>>>>>> 73eb6c283269d3698b418665a84bf3716bb15e61
