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
 * @file hal_watchdog.h
 * @brief Portable HAL interface for the independent and window watchdogs.
 *
 * @details
 * ### The independent watchdog (`hal_watchdog_*`)
 * A free-running down-counter on its own low-speed oscillator that resets the
 * part if firmware stops kicking it. Independent is the operative word: it does
 * not share the system clock, so it still fires when the PLL has died or an
 * interrupt storm has starved the main loop — the failures a software timer
 * cannot catch because it is a casualty of them.
 *
 * **It cannot be stopped.** Once @ref hal_watchdog_start returns, only a reset
 * turns it off. That is the point, and it is also why nothing here offers a
 * stop: an API that could be talked out of watching is not a watchdog. Test
 * code has to plan around this — starting one part-way through a suite means
 * the suite is now on a deadline.
 *
 * The timeout is quantised by a prescaler and a 12-bit reload, and the
 * oscillator behind it is an RC good to a few percent over temperature. Ask for
 * what you want, then read @ref hal_watchdog_get_timeout_ms for what you got,
 * and kick at a comfortable fraction of it — a third is a reasonable habit.
 *
 * ### The window watchdog (`hal_wwdg_*`)
 * A different contract: kicking too *early* is a fault as much as kicking too
 * late. It catches a loop that has started spinning free — running fast, or out
 * of sequence — which the independent watchdog is blind to because a runaway
 * loop kicks perfectly well. Clocked from the peripheral bus, so it is a
 * program-flow check, not a clock-failure check. The two answer different
 * questions and can run together.
 *
 * ### Typical usage
 * @code
 * hal_watchdog_start(1000);                  // ~1 s, no way back
 * uint32_t period = hal_watchdog_get_timeout_ms();
 * for (;;) {
 *   do_work();
 *   hal_watchdog_kick();                     // comfortably inside `period`
 * }
 * @endcode
 */

#ifndef HAL_WATCHDOG_H
#define HAL_WATCHDOG_H

/**
 * @defgroup HAL_WATCHDOG Watchdog
 * @ingroup HAL_DRIVERS
 * @brief Independent and window watchdog timers.
 * @{
 */

#include "common/hal_status.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the independent watchdog. Irreversible.
 *
 * The requested timeout is rounded to what the prescaler and reload can
 * express — always to a value at least as long as asked for, so a caller that
 * kicks on its own schedule is never bitten early by rounding. Ask for longer
 * than the hardware can reach and the call fails rather than silently arming a
 * much shorter watchdog.
 *
 * @param timeout_ms Milliseconds of silence tolerated before a reset.
 * @retval HAL_OK                 Running. It cannot be stopped from here on.
 * @retval HAL_ERR_INVALID_ARG    Zero, or longer than the hardware can express.
 * @retval HAL_ERR_TIMEOUT        The watchdog clock never became ready.
 */
hal_status_t hal_watchdog_start(uint32_t timeout_ms);

/**
 * @brief Reload the counter. Call comfortably more often than the timeout.
 *
 * @retval HAL_OK                  Reloaded.
 * @retval HAL_ERR_NOT_INITIALIZED The watchdog is not running.
 */
hal_status_t hal_watchdog_kick(void);

/**
 * @brief The timeout actually programmed, after rounding.
 *
 * @return Milliseconds, or 0 if the watchdog is not running. Nominal: the
 *         oscillator behind it is an RC, so the real interval drifts with
 *         temperature and supply by a few percent.
 */
uint32_t hal_watchdog_get_timeout_ms(void);

/**
 * @brief Whether this driver started the independent watchdog.
 *
 * Reflects @ref hal_watchdog_start, not the hardware: a part configured to
 * arm its watchdog from an option byte before firmware runs is counting down
 * with nothing here to read it from, and reports false.
 */
bool hal_watchdog_is_running(void);

/**
 * @brief Longest timeout this target's independent watchdog can express.
 *
 * @return Milliseconds. Lets a caller clamp rather than guess at the boundary.
 */
uint32_t hal_watchdog_max_timeout_ms(void);

#if NAVHAL_CONFIG_DRV_WWDG

/**
 * @brief Start the window watchdog. Irreversible.
 *
 * Kicking is only legal once the counter has fallen into the window: reload
 * before that and the part resets exactly as if it had timed out. So
 * `window_ms` is a floor on the kick period and `timeout_ms` is the ceiling,
 * and a caller has to sit between them.
 *
 * @param timeout_ms Ceiling: silence longer than this resets the part.
 * @param window_ms  Floor: kicking sooner than this after the last kick resets
 *                   it too. Must be less than @p timeout_ms.
 * @retval HAL_OK                 Running.
 * @retval HAL_ERR_INVALID_ARG    Zero timeout, @p window_ms not below
 *                                @p timeout_ms, or a value the counter cannot
 *                                express.
 */
hal_status_t hal_wwdg_start(uint32_t timeout_ms, uint32_t window_ms);

/**
 * @brief Reload the window watchdog. Only legal inside the window.
 *
 * @retval HAL_OK                  Reloaded.
 * @retval HAL_ERR_NOT_INITIALIZED The window watchdog is not running.
 */
hal_status_t hal_wwdg_kick(void);

/**
 * @brief Whether the counter has fallen into the window yet.
 *
 * A caller that cannot guarantee its own timing can poll this instead of
 * risking an early kick.
 *
 * @return true when @ref hal_wwdg_kick is currently legal.
 */
bool hal_wwdg_window_open(void);

/**
 * @brief Whether the window watchdog is running.
 */
bool hal_wwdg_is_running(void);

#endif /* NAVHAL_CONFIG_DRV_WWDG */

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* HAL_WATCHDOG_H */
