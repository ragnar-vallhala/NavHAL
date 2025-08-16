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
#include "core/cortex-m4/uart.h"
#include "utils/timer_types.h"
#include <stdint.h>

/* Global variables */
static volatile uint64_t systick_ticks = 0;     ///< Global system tick counter
static volatile uint32_t tick_duration_us = 1;  ///< SysTick tick duration in us
static volatile uint32_t tick_reload_value = 0; ///< Current SysTick reload value

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
 */
static void _enable_timer_rcc(hal_timer_t timer) {
  switch (timer) {
  case TIM1: RCC_APB2ENR |= (1 << RCC_APB2ENR_TIM1_OFFSET); break;
  /* ... other timers ... */
  }
}

/**
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
}

/**
 * @brief Start timer counter
 * @param timer Timer identifier
 */
void timer_start(hal_timer_t timer) {
  /* ... implementation ... */
}

/**
 * @brief Stop timer counter
 * @param timer Timer identifier
 */
void timer_stop(hal_timer_t timer) {
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
}

/**
 * @brief Disable timer channel output
 * @param timer Timer identifier
 * @param channel Channel number (1-4)
 */
void timer_disable_channel(hal_timer_t timer, uint32_t channel) {
  /* ... implementation ... */
}