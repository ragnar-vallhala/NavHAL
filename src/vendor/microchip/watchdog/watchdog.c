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
 * @file watchdog.c
 * @brief ATmega328P implementation of the independent watchdog in
 *        hal_watchdog.h.
 *
 * @details
 * The AVR watchdog runs from its own ~128 kHz RC oscillator, so it survives a
 * stalled system clock the same way the STM32 IWDG does. Unlike the IWDG its
 * interval is not a prescaler over a reload but one of ten fixed steps, so the
 * rounding here is a table lookup rather than arithmetic.
 *
 * The ATmega328P has no window watchdog, so hal_wwdg_* is absent on this
 * target — CONFIG_DRV_WWDG is not offered for AVR.
 */

/* The TEST build globs every vendor source regardless of Kconfig, so the gate
 * has to be in the file as well as in CMake — otherwise the driver links into
 * builds that never asked for it. */
#if NAVHAL_CONFIG_DRV_WATCHDOG

#include "common/hal_watchdog.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>

/**
 * @brief The ten intervals WDP[3:0] can express, in milliseconds.
 *
 * Index is the avr-libc WDTO_* value. Nominal at 5 V: the oscillator behind
 * them shifts substantially with supply voltage and temperature, far more than
 * a crystal-backed timer would.
 */
static const uint16_t wdt_steps_ms[] = {16U,   32U,   64U,   125U,  250U,
                                        500U,  1000U, 2000U, 4000U, 8000U};

#define WDT_STEP_COUNT (sizeof(wdt_steps_ms) / sizeof(wdt_steps_ms[0]))

static uint16_t wdt_timeout_ms;
static uint8_t wdt_running;

uint32_t hal_watchdog_max_timeout_ms(void) {
  return wdt_steps_ms[WDT_STEP_COUNT - 1U];
}

hal_status_t hal_watchdog_start(uint32_t timeout_ms) {
  if (timeout_ms == 0U || timeout_ms > hal_watchdog_max_timeout_ms())
    return HAL_ERR_INVALID_ARG;

  /* First step at least as long as the request — never round down, or a caller
   * kicking on its own schedule gets bitten early. */
  uint8_t step = 0U;
  while (step < WDT_STEP_COUNT - 1U && wdt_steps_ms[step] < timeout_ms)
    step++;

  /* wdt_enable performs the WDCE timed sequence, which has to complete within
   * four cycles of its start — an interrupt landing in the middle makes the
   * write silently do nothing. */
  const uint8_t sreg = SREG;
  cli();
  wdt_reset();
  wdt_enable(step);
  SREG = sreg;

  wdt_timeout_ms = wdt_steps_ms[step];
  wdt_running = 1U;
  return HAL_OK;
}

hal_status_t hal_watchdog_kick(void) {
  if (!wdt_running)
    return HAL_ERR_NOT_INITIALIZED;
  wdt_reset();
  return HAL_OK;
}

uint32_t hal_watchdog_get_timeout_ms(void) { return wdt_timeout_ms; }

bool hal_watchdog_is_running(void) { return wdt_running != 0U; }

#endif /* NAVHAL_CONFIG_DRV_WATCHDOG */
