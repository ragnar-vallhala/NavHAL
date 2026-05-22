/**
 * @file timebase.c
 * @brief SysTick-backed timebase driver for ARMv7E-M cores (Cortex-M4).
 *
 * @details
 * The timebase is the periodic millisecond/microsecond tick used by the rest
 * of the HAL for `hal_delay_*` busy-waits, monotonic millis/micros, and any
 * user-registered tick callback. It is implemented on top of the Cortex-M
 * core SysTick peripheral and so lives in the arch layer — every ARMv7E-M
 * target gets it for free, independent of which vendor IP timers are present.
 *
 * The AHB clock for the SysTick reload is queried via `hal_clock_get_ahbclk()`
 * from the vendor clock driver, which is the only outward dependency this
 * file has.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "navhal_port_clock.h"
#include "navhal_port_timer.h"
#include <stdint.h>

/**
 * @brief Global timebase tick counter (incremented in SysTick_Handler).
 * @note Unit: ticks; tick duration is set by hal_timebase_init().
 */
static volatile uint32_t systick_ticks = 0;

/** @brief Timebase tick duration in microseconds (1 us until init). */
static volatile uint32_t tick_duration_us = 1;

/** @brief SysTick reload value (24-bit) currently configured. */
static volatile uint32_t tick_reload_value = 0;

/** @brief User callback invoked on every timebase tick. */
static hal_timebase_callback_t timebase_callback = 0;

/*
 * @brief Initialize the timebase (SysTick) to generate periodic ticks.
 * (API doc lives in common/hal_timer.h; this is an implementation note.)
 *
 * @param tick_us Tick period in microseconds; must be non-zero.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG if @p tick_us is 0.
 *
 * @note The SysTick reload is 24-bit; the computed reload value is clipped
 *       to 24 bits. Configures the clock source, enables the SysTick
 *       interrupt and starts the timer.
 */
hal_status_t hal_timebase_init(uint32_t tick_us) {
  if (tick_us == 0)
    return HAL_ERR_INVALID_ARG;

  // systick interrupt is not under the NVIC
  systick_ticks = 0;
  tick_duration_us = tick_us;
  uint32_t ahbclk = hal_clock_get_ahbclk();
  uint64_t reload_value = (((uint64_t)ahbclk * tick_us) / 1000000ULL) - 1;
  reload_value &= 0xffffff; // set the reload value as 24bit only
  uint32_t reload = (uint32_t)reload_value;
  tick_reload_value = reload;
  SYST_RVR = reload; // Set reload value
  SYST_CVR = 0;      // Clear current value
  SYST_CSR = (1 << SYST_CSR_EN_BIT) | (1 << SYST_CSR_TICKINT_BIT) |
             (1 << SYST_CSR_CLKSOURCE_BIT); // Enable | TickInt | ClkSource
  return HAL_OK;
}

/**
 * @brief Register a callback invoked on every timebase tick.
 * @param cb Callback function, or NULL to clear.
 * @return ::HAL_OK.
 */
hal_status_t hal_timebase_set_callback(hal_timebase_callback_t cb) {
  timebase_callback = cb;
  return HAL_OK;
}

/**
 * @brief Busy-wait for the specified number of microseconds.
 *
 * @param us Number of microseconds to delay.
 *
 * @note Blocking busy-wait using the timebase tick; waits at least one tick
 *       if the requested delay is smaller than the tick duration.
 */
void hal_delay_us(uint32_t us) {
  uint32_t ticks_needed = us / hal_timebase_get_tick_duration_us();
  if (ticks_needed == 0)
    ticks_needed = 1;
  uint32_t start = hal_timebase_get_tick();
  while (hal_timebase_get_tick() - start < ticks_needed)
    __asm__ volatile("nop"); // insert noops in bw
}

/**
 * @brief Busy-wait for the specified number of milliseconds.
 * @param ms Number of milliseconds to delay.
 */
void hal_delay_ms(uint32_t ms) { hal_delay_us(ms * 1000); }

/** @brief Return the current timebase tick count. */
uint32_t hal_timebase_get_tick(void) { return systick_ticks; }

/** @brief Return the configured tick duration in microseconds. */
uint32_t hal_timebase_get_tick_duration_us(void) { return tick_duration_us; }

/** @brief Return the SysTick reload value (24-bit). */
uint32_t hal_timebase_get_reload_value(void) { return tick_reload_value; }

/** @brief Return elapsed time since timebase start, in milliseconds. */
uint32_t hal_timebase_get_millis(void) {
  return hal_timebase_get_micros() / 1000;
}

/** @brief Return elapsed time since timebase start, in microseconds. */
uint32_t hal_timebase_get_micros(void) {
  return hal_timebase_get_tick() * hal_timebase_get_tick_duration_us();
}

/**
 * @brief SysTick exception handler — increments the tick counter and
 *        invokes the registered timebase callback, if any.
 */
#ifndef SUBMODULE
void SysTick_Handler(void) {
  systick_ticks++;
  if (timebase_callback)
    timebase_callback();
}
#endif
