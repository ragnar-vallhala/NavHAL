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
 */

#include "navhal_port_clock.h"
#include "internal/hal_timebase_ops.h"

/* Forward declarations: several of these call each other (micros from tick,
 * delays from both), and they are static now. */
static hal_status_t armv7em_timebase_init(uint32_t tick_us);
static uint32_t armv7em_timebase_get_tick(void);
static uint32_t armv7em_timebase_get_tick_duration_us(void);
static uint32_t armv7em_timebase_get_reload_value(void);
static uint32_t armv7em_timebase_get_micros(void);
static uint32_t armv7em_timebase_get_millis(void);
static void armv7em_timebase_delay_us(uint32_t us);
static void armv7em_timebase_delay_ms(uint32_t ms);
static hal_status_t armv7em_timebase_set_callback(hal_timebase_callback_t cb);
#include "navhal_port_timer.h"
#include <stdint.h>

/**
 * @brief Global timebase tick counter (incremented in SysTick_Handler).
 * @note Unit: ticks; tick duration is set by armv7em_timebase_init().
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
static hal_status_t armv7em_timebase_init(uint32_t tick_us) {
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
static hal_status_t armv7em_timebase_set_callback(hal_timebase_callback_t cb) {
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
static void armv7em_timebase_delay_us(uint32_t us) {
  uint32_t ticks_needed = us / armv7em_timebase_get_tick_duration_us();
  if (ticks_needed == 0)
    ticks_needed = 1;
  uint32_t start = armv7em_timebase_get_tick();
  while (armv7em_timebase_get_tick() - start < ticks_needed)
    __asm__ volatile("nop"); // insert noops in bw
}

/**
 * @brief Busy-wait for the specified number of milliseconds.
 * @param ms Number of milliseconds to delay.
 */
static void armv7em_timebase_delay_ms(uint32_t ms) { armv7em_timebase_delay_us(ms * 1000); }

/** @brief Return the current timebase tick count. */
static uint32_t armv7em_timebase_get_tick(void) { return systick_ticks; }

/** @brief Return the configured tick duration in microseconds. */
static uint32_t armv7em_timebase_get_tick_duration_us(void) { return tick_duration_us; }

/** @brief Return the SysTick reload value (24-bit). */
static uint32_t armv7em_timebase_get_reload_value(void) { return tick_reload_value; }

/** @brief Return elapsed time since timebase start, in milliseconds. */
static uint32_t armv7em_timebase_get_millis(void) {
  return armv7em_timebase_get_micros() / 1000;
}

/** @brief Return elapsed time since timebase start, in microseconds. */
static uint32_t armv7em_timebase_get_micros(void) {
  return armv7em_timebase_get_tick() * armv7em_timebase_get_tick_duration_us();
}

/**
 * @brief Advance the timebase by one tick and run the registered callback.
 *
 * This is the body of the SysTick ISR, factored out so an embedding RTOS that
 * owns the SysTick vector (SUBMODULE builds, where the handler below is
 * compiled out) can keep the NavHAL timebase alive by calling this from its
 * own SysTick_Handler. Without it `systick_ticks` never advances and every
 * hal_delay_*() busy-wait spins forever.
 */
void hal_timebase_tick(void) {
  systick_ticks++;
  if (timebase_callback)
    timebase_callback();
}

/**
 * @brief SysTick exception handler — drives the timebase tick.
 */
#ifndef SUBMODULE
void SysTick_Handler(void) { hal_timebase_tick(); }
#endif


/** @brief The ARMv7E-M SysTick timebase backend. */
const hal_timebase_ops_t _hal_timebase_ops = {
    .init = armv7em_timebase_init,
    .get_tick = armv7em_timebase_get_tick,
    .get_tick_duration_us = armv7em_timebase_get_tick_duration_us,
    .get_reload_value = armv7em_timebase_get_reload_value,
    .get_micros = armv7em_timebase_get_micros,
    .get_millis = armv7em_timebase_get_millis,
    .delay_us = armv7em_timebase_delay_us,
    .delay_ms = armv7em_timebase_delay_ms,
    .set_callback = armv7em_timebase_set_callback,
};
