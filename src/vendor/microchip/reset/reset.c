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
 * @file reset.c
 * @brief ATmega328P implementation of hal_reset.h.
 *
 * @details
 * MCUSR carries the reset flags, and the AVR has no reset-request register at
 * all — the watchdog is how you reset the part deliberately, which makes the
 * two halves of this API share hardware here in a way they do not on Cortex-M.
 */

/* The TEST build globs every vendor source regardless of Kconfig, so the gate
 * has to be in the file as well as in CMake — otherwise the driver links into
 * builds that never asked for it. */
#if NAVHAL_CONFIG_DRV_RESET

#include "common/hal_reset.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h>

static uint32_t latched_cause;
static uint8_t cause_valid;

hal_status_t hal_reset_init(void) {
  if (cause_valid)
    return HAL_OK;

  const uint8_t mcusr = MCUSR;
  uint32_t cause = HAL_RESET_CAUSE_UNKNOWN;

  if (mcusr & (1 << PORF))
    cause |= HAL_RESET_CAUSE_POWER_ON;
  if (mcusr & (1 << EXTRF))
    cause |= HAL_RESET_CAUSE_PIN;
  if (mcusr & (1 << BORF))
    cause |= HAL_RESET_CAUSE_BROWNOUT;
  if (mcusr & (1 << WDRF))
    cause |= HAL_RESET_CAUSE_WATCHDOG;

  latched_cause = cause;
  cause_valid = 1;

  /* Clearing WDRF is not housekeeping, it is the thing that stops a boot loop.
   * A watchdog reset leaves WDE set and the timer running at its shortest
   * interval, and the datasheet forbids clearing WDE while WDRF is still set —
   * so a part that resets before reaching here resets again, forever. */
  MCUSR = 0;
  wdt_disable();

  return HAL_OK;
}

uint32_t hal_reset_get_cause(void) { return latched_cause; }

hal_status_t hal_system_reset(void) {
  /* No SYSRESETREQ equivalent: arm the shortest watchdog and stop feeding it.
   * The next boot therefore reports HAL_RESET_CAUSE_WATCHDOG rather than
   * _SOFTWARE — the hardware keeps no separate flag for "firmware asked", and
   * inventing one would mean claiming to know something MCUSR does not say. */
  cli();
  wdt_enable(WDTO_15MS);
  for (;;) {
  }
}

#endif /* NAVHAL_CONFIG_DRV_RESET */
