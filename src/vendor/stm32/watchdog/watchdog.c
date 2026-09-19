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
 * @brief STM32 implementation of hal_watchdog.h — IWDG and WWDG.
 */

/* The TEST build globs every vendor source regardless of Kconfig, so the gate
 * has to be in the file as well as in CMake — otherwise the driver links into
 * builds that never asked for it. */
#if NAVHAL_CONFIG_DRV_WATCHDOG

#include "common/hal_watchdog.h"
#include "internal/hal_watchdog_ops.h"
#include "common/hal_clock.h"
#include "family/rcc_reg.h"
#include "family/wdg_reg.h"

/* An IWDG register write crosses into the LSI clock domain and takes a handful
 * of 32 kHz cycles (~30 µs each) to land. This only has to outlast that at
 * whatever core clock we happen to be running, so it is generous rather than
 * calculated — the cost of it being too large is a slower failure report on a
 * dead LSI, which is not a hot path. */
#define IWDG_UPDATE_SPINS 2000000UL

static uint32_t iwdg_timeout_ms;
static uint8_t iwdg_running;

static uint32_t stm32_watchdog_max_timeout_ms(void) {
  /* Slowest divider against a full 12-bit reload. */
  return (uint32_t)(((IWDG_RLR_MASK + 1UL) * (4UL << IWDG_PR_MAX) * 1000UL) /
                    IWDG_LSI_HZ);
}

static hal_status_t stm32_watchdog_start(uint32_t timeout_ms) {
  if (timeout_ms == 0U || timeout_ms > stm32_watchdog_max_timeout_ms())
    return HAL_ERR_INVALID_ARG;

  /* LSI cycles the request is worth, rounded up: a caller that asks for 1 s
   * must never be bitten at 0.99 s. */
  const uint32_t ticks = (timeout_ms * IWDG_LSI_HZ + 999U) / 1000U;

  /* Take the finest prescaler that still fits the reload in 12 bits — the
   * coarser the divider, the more of the request rounding throws away. */
  uint32_t pr = 0U;
  uint32_t div = 4U;
  while (((ticks + div - 1U) / div) > (IWDG_RLR_MASK + 1U) && pr < IWDG_PR_MAX) {
    pr++;
    div <<= 1;
  }

  uint32_t reload = (ticks + div - 1U) / div;
  if (reload == 0U)
    reload = 1U; /* the shortest the hardware can express */
  if (reload > IWDG_RLR_MASK + 1U)
    return HAL_ERR_INVALID_ARG;

  /* START first: it is what turns the LSI on, and the status bits below never
   * clear until the watchdog's own clock is running. The default reload is
   * active for the microseconds this takes to reprogram. */
  IWDG->KR = IWDG_KEY_START;
  IWDG->KR = IWDG_KEY_UNLOCK;
  IWDG->PR = pr;
  IWDG->RLR = reload - 1U;

  uint32_t spins = IWDG_UPDATE_SPINS;
  while (IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) {
    if (--spins == 0U)
      return HAL_ERR_TIMEOUT; /* running, but on the default reload */
  }

  IWDG->KR = IWDG_KEY_RELOAD; /* also re-locks PR and RLR */

  iwdg_timeout_ms = (uint32_t)((reload * div * 1000UL) / IWDG_LSI_HZ);
  iwdg_running = 1U;
  return HAL_OK;
}

static hal_status_t stm32_watchdog_kick(void) {
  if (!iwdg_running)
    return HAL_ERR_NOT_INITIALIZED;
  IWDG->KR = IWDG_KEY_RELOAD;
  return HAL_OK;
}

static uint32_t stm32_watchdog_get_timeout_ms(void) { return iwdg_timeout_ms; }

static bool stm32_watchdog_is_running(void) { return iwdg_running != 0U; }

#if NAVHAL_CONFIG_DRV_WWDG

static uint8_t wwdg_reload;  /**< T value written on every kick. */
static uint8_t wwdg_window;  /**< W value; kicking above it resets the part. */
static uint8_t wwdg_running;

hal_status_t hal_wwdg_start(uint32_t timeout_ms, uint32_t window_ms) {
  if (timeout_ms == 0U || window_ms >= timeout_ms)
    return HAL_ERR_INVALID_ARG;

  const uint32_t pclk1 = hal_clock_get_apb1clk();
  if (pclk1 == 0U)
    return HAL_ERR_NOT_INITIALIZED;

  /* The counter only spans 0x40..0x7F, so 64 ticks is the whole range. Walk
   * the timebase up until the request fits, finest first. */
  uint32_t tb = 0U;
  uint32_t tick_hz = 0U;
  uint32_t counts = 0U;
  for (; tb <= 3U; tb++) {
    tick_hz = pclk1 / (WWDG_PCLK_DIV << tb);
    if (tick_hz == 0U)
      return HAL_ERR_INVALID_ARG;
    counts = (timeout_ms * tick_hz + 999U) / 1000U;
    if (counts <= 64U)
      break;
  }
  if (counts == 0U || counts > 64U)
    return HAL_ERR_INVALID_ARG; /* longer than this PCLK1 can express */

  const uint32_t t = (WWDG_CR_T_MIN - 1U) + counts; /* 0x3F + counts */

  /* Round the window down: opening it early costs a caller nothing, opening it
   * late resets the part. */
  const uint32_t wcounts = (window_ms * tick_hz) / 1000U;
  if (wcounts >= counts)
    return HAL_ERR_INVALID_ARG; /* the window would never open */
  const uint32_t w = t - wcounts;

  RCC->APB1ENR |= RCC_APB1ENR_WWDGEN;
  WWDG->CFR = (uint32_t)((tb << WWDG_CFR_WDGTB_BIT) | (w & WWDG_CFR_W_MASK));
  /* WDGA starts it and nothing but a reset clears it, so this is the point of
   * no return — the window has to be configured before it, not after. */
  WWDG->CR = WWDG_CR_WDGA | (t & WWDG_CR_T_MASK);

  wwdg_reload = (uint8_t)t;
  wwdg_window = (uint8_t)w;
  wwdg_running = 1U;
  return HAL_OK;
}

hal_status_t hal_wwdg_kick(void) {
  if (!wwdg_running)
    return HAL_ERR_NOT_INITIALIZED;
  /* WDGA is set-only, so writing the counter alone cannot switch it off. */
  WWDG->CR = wwdg_reload;
  return HAL_OK;
}

bool hal_wwdg_window_open(void) {
  if (!wwdg_running)
    return false;
  return (WWDG->CR & WWDG_CR_T_MASK) <= wwdg_window;
}

bool hal_wwdg_is_running(void) { return wwdg_running != 0U; }

#endif /* NAVHAL_CONFIG_DRV_WWDG */


/** @brief The STM32 watchdog backend. */
const hal_watchdog_ops_t _hal_watchdog_ops = {
    .start = stm32_watchdog_start,
    .kick = stm32_watchdog_kick,
    .get_timeout_ms = stm32_watchdog_get_timeout_ms,
    .is_running = stm32_watchdog_is_running,
    .max_timeout_ms = stm32_watchdog_max_timeout_ms,
};

#endif /* NAVHAL_CONFIG_DRV_WATCHDOG */
