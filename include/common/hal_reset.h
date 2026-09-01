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
 * @file hal_reset.h
 * @brief Portable HAL interface for resetting the MCU and asking why it last
 *        reset.
 *
 * @details
 * Two halves of the same question. @ref hal_system_reset restarts the part;
 * @ref hal_reset_get_cause tells the next boot what restarted it — a power-on,
 * a pin, a brown-out, a watchdog bite, or firmware asking. Without the second,
 * a device that recovers from a hang cannot tell anyone that it did.
 *
 * ### Reading the cause
 * The hardware flags are **sticky**: they accumulate across resets until
 * explicitly cleared, so a device that never clears them reports the union of
 * everything that has ever happened to it. @ref hal_reset_init latches the
 * flags and clears them in one step, which is why it should run early and
 * exactly once. Every later @ref hal_reset_get_cause returns that latched
 * value, so the order of calls after init does not matter.
 *
 * A cause is a bitmask, not an enum value: a brown-out that also trips the pin
 * reset line sets both, and reporting only the first would lose that.
 *
 * ### Typical usage
 * @code
 * hal_reset_init();
 * if (hal_reset_get_cause() & HAL_RESET_CAUSE_WATCHDOG) {
 *   log("recovered from a hang");   // the previous run stopped kicking
 * }
 * ...
 * hal_system_reset();               // does not return
 * @endcode
 */

#ifndef HAL_RESET_H
#define HAL_RESET_H

/**
 * @defgroup HAL_RESET Reset
 * @ingroup HAL_DRIVERS
 * @brief MCU reset and reset-cause reporting.
 * @{
 */

#include "common/hal_status.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Why the MCU last reset, as a bitmask.
 *
 * More than one bit can be set: the flags are latched independently and a
 * single event can trip several. ::HAL_RESET_CAUSE_UNKNOWN is the value when
 * no flag was set at all, which on most parts means someone already cleared
 * them before @ref hal_reset_init ran.
 */
typedef enum {
  HAL_RESET_CAUSE_UNKNOWN = 0U,         /**< No flag was latched. */
  HAL_RESET_CAUSE_POWER_ON = 1U << 0,   /**< Power-on / power-down reset. */
  HAL_RESET_CAUSE_PIN = 1U << 1,        /**< External reset pin (NRST). */
  HAL_RESET_CAUSE_BROWNOUT = 1U << 2,   /**< Supply dipped below the BOR level. */
  HAL_RESET_CAUSE_SOFTWARE = 1U << 3,   /**< Firmware asked (@ref hal_system_reset). */
  HAL_RESET_CAUSE_WATCHDOG = 1U << 4,   /**< Independent watchdog timed out. */
  HAL_RESET_CAUSE_WINDOW_WATCHDOG = 1U << 5, /**< Window watchdog timed out. */
  HAL_RESET_CAUSE_LOW_POWER = 1U << 6,  /**< Illegal low-power-mode entry. */
} hal_reset_cause_t;

/**
 * @brief Latch the reset-cause flags and clear them in the hardware.
 *
 * Call once, early. Safe to call again — the latched value is only taken from
 * the hardware the first time, so a second call cannot lose the cause.
 *
 * @return ::HAL_OK.
 */
hal_status_t hal_reset_init(void);

/**
 * @brief Why the MCU last reset.
 *
 * @return A bitmask of ::hal_reset_cause_t. ::HAL_RESET_CAUSE_UNKNOWN if
 *         @ref hal_reset_init has not run, or if it found no flags set.
 */
uint32_t hal_reset_get_cause(void);

/**
 * @brief Reset the MCU immediately.
 *
 * Does not return. Nothing is flushed first: a caller that cares about pending
 * writes (a UART still draining, a flash program in flight) has to wait for
 * them itself before calling.
 *
 * The next boot reports ::HAL_RESET_CAUSE_SOFTWARE.
 *
 * @return Does not return. The declared ::hal_status_t exists so the call sits
 *         in an expression the same way every other HAL entry point does.
 */
hal_status_t hal_system_reset(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* HAL_RESET_H */
