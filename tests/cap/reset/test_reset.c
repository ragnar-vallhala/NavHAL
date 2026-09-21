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
 * @file test_reset.c
 * @brief On-target tests for the reset driver.
 *
 * @details
 * Device-free, and deliberately does not reset the board: a suite that called
 * hal_system_reset() would restart into itself and never finish. That the
 * reset works, and that the cause survives to report it, is what
 * samples/cortex-m/36_hal_watchdog demonstrates end to end.
 *
 * What is checked here is the latch: the hardware flags are sticky and
 * hal_reset_init() clears them, so the value has to be captured once and stay
 * readable afterwards. Getting that wrong turns the cause into "unknown" for
 * every caller that asks second, which is the kind of bug that only shows up
 * in the field.
 */

#include "test_reset.h"
#include "common/hal_features.h"
#include "navtest/navtest.h"
#include <stdint.h>

#if NAVHAL_CONFIG_DRV_RESET
#include "common/hal_reset.h"

void test_reset_init_returns_ok(void) {
  TEST_ASSERT_EQUAL_UINT32(HAL_OK, hal_reset_init());
}

void test_reset_cause_is_a_known_bit(void) {
  hal_reset_init();
  const uint32_t cause = hal_reset_get_cause();
  const uint32_t known =
      HAL_RESET_CAUSE_POWER_ON | HAL_RESET_CAUSE_PIN |
      HAL_RESET_CAUSE_BROWNOUT | HAL_RESET_CAUSE_SOFTWARE |
      HAL_RESET_CAUSE_WATCHDOG | HAL_RESET_CAUSE_WINDOW_WATCHDOG |
      HAL_RESET_CAUSE_LOW_POWER;
  /* Which bits are set depends on how the board got here — a debugger reset,
   * a power cycle, the button. That none of them is outside the enum is what
   * catches a register-bit mapping that has drifted. */
  TEST_ASSERT_EQUAL_UINT32(0u, cause & ~known);
}

void test_reset_cause_survives_the_clear(void) {
  /* The whole point of latching. init() clears the sticky hardware flags, so
   * an implementation that read them on demand instead would report the real
   * cause once and zero forever after. */
  hal_reset_init();
  const uint32_t first = hal_reset_get_cause();
  hal_reset_init(); /* second call must not overwrite the latch */
  TEST_ASSERT_EQUAL_UINT32(first, hal_reset_get_cause());
  TEST_ASSERT_EQUAL_UINT32(first, hal_reset_get_cause());
}

void test_reset_cause_is_stable_across_reads(void) {
  hal_reset_init();
  const uint32_t a = hal_reset_get_cause();
  for (int i = 0; i < 8; i++)
    TEST_ASSERT_EQUAL_UINT32(a, hal_reset_get_cause());
}

/* -------------------- Suite -------------------- */

NAVTEST_CASE_DECL(test_reset_init_returns_ok);
NAVTEST_CASE_DECL(test_reset_cause_is_a_known_bit);
NAVTEST_CASE_DECL(test_reset_cause_survives_the_clear);
NAVTEST_CASE_DECL(test_reset_cause_is_stable_across_reads);

static const navtest_case_t reset_cases[] = {
    NAVTEST_CASE(test_reset_init_returns_ok),
    NAVTEST_CASE(test_reset_cause_is_a_known_bit),
    NAVTEST_CASE(test_reset_cause_survives_the_clear),
    NAVTEST_CASE(test_reset_cause_is_stable_across_reads),
};

const navtest_suite_t test_reset_suite = {
    .name = "RESET",
    .cases = reset_cases,
    .count = sizeof(reset_cases) / sizeof(reset_cases[0]),
    .between = NULL,
};

#endif /* NAVHAL_CONFIG_DRV_RESET */
