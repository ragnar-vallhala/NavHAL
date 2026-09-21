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
 * @file test_watchdog.c
 * @brief On-target tests for the watchdog driver.
 *
 * @details
 * **Nothing here starts a watchdog.** Neither the IWDG nor the WWDG can be
 * stopped once armed, so a suite that started one would be running against a
 * deadline for every case after it, and a slow assert further down would look
 * like a watchdog bug. Proving that it bites belongs in a sample that expects
 * to be reset — samples/cortex-m/36_hal_watchdog — where the reset is the
 * result rather than a casualty.
 *
 * What is left is still the part most likely to be wrong: the arithmetic that
 * turns a millisecond request into a prescaler and a reload, and the refusals
 * that stop a caller arming something other than what they asked for.
 */

#include "test_watchdog.h"
#include "common/hal_features.h"
#include "navtest/navtest.h"
#include <stdint.h>

#if NAVHAL_CONFIG_DRV_WATCHDOG
#include "common/hal_watchdog.h"

void test_watchdog_not_running_before_start(void) {
  TEST_ASSERT_FALSE(hal_watchdog_is_running());
  TEST_ASSERT_EQUAL_UINT32(0u, hal_watchdog_get_timeout_ms());
}

void test_watchdog_kick_refused_before_start(void) {
  /* A caller that kicks a watchdog it never started has a bug, and silently
   * returning OK would hide it until the day the start is added. */
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_NOT_INITIALIZED, hal_watchdog_kick());
}

void test_watchdog_rejects_zero_timeout(void) {
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG, hal_watchdog_start(0));
  TEST_ASSERT_FALSE(hal_watchdog_is_running());
}

void test_watchdog_rejects_timeout_past_the_maximum(void) {
  /* Arming a much shorter watchdog than asked for is the dangerous failure,
   * so an impossible request has to be refused rather than clamped. */
  const uint32_t max = hal_watchdog_max_timeout_ms();
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG, hal_watchdog_start(max + 1u));
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG, hal_watchdog_start(0xFFFFFFFFu));
  TEST_ASSERT_FALSE(hal_watchdog_is_running());
}

void test_watchdog_max_timeout_is_sane(void) {
  /* Bounds, not an exact figure: the STM32 IWDG reaches ~32.7 s and the AVR
   * WDT 8 s, and pinning either exactly would make this a change-detector. */
  const uint32_t max = hal_watchdog_max_timeout_ms();
  TEST_ASSERT_TRUE(max >= 1000u);
  TEST_ASSERT_TRUE(max <= 60000u);
}

#if NAVHAL_CONFIG_DRV_WWDG

void test_wwdg_not_running_before_start(void) {
  TEST_ASSERT_FALSE(hal_wwdg_is_running());
  TEST_ASSERT_FALSE(hal_wwdg_window_open());
}

void test_wwdg_kick_refused_before_start(void) {
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_NOT_INITIALIZED, hal_wwdg_kick());
}

void test_wwdg_rejects_bad_window(void) {
  /* A window at or past the timeout can never open, so every kick would reset
   * the part — the caller has inverted the two arguments. */
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG, hal_wwdg_start(10u, 10u));
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG, hal_wwdg_start(10u, 20u));
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG, hal_wwdg_start(0u, 0u));
  TEST_ASSERT_FALSE(hal_wwdg_is_running());
}

void test_wwdg_rejects_timeout_it_cannot_express(void) {
  /* The counter spans 64 ticks of PCLK1/4096 at most, so the WWDG tops out in
   * the tens of milliseconds. Seconds are the independent watchdog's job. */
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG, hal_wwdg_start(10000u, 1u));
  TEST_ASSERT_FALSE(hal_wwdg_is_running());
}

#endif /* NAVHAL_CONFIG_DRV_WWDG */

/* -------------------- Suite -------------------- */

NAVTEST_CASE_DECL(test_watchdog_not_running_before_start);
NAVTEST_CASE_DECL(test_watchdog_kick_refused_before_start);
NAVTEST_CASE_DECL(test_watchdog_rejects_zero_timeout);
NAVTEST_CASE_DECL(test_watchdog_rejects_timeout_past_the_maximum);
NAVTEST_CASE_DECL(test_watchdog_max_timeout_is_sane);
#if NAVHAL_CONFIG_DRV_WWDG
NAVTEST_CASE_DECL(test_wwdg_not_running_before_start);
NAVTEST_CASE_DECL(test_wwdg_kick_refused_before_start);
NAVTEST_CASE_DECL(test_wwdg_rejects_bad_window);
NAVTEST_CASE_DECL(test_wwdg_rejects_timeout_it_cannot_express);
#endif

static const navtest_case_t watchdog_cases[] = {
    NAVTEST_CASE(test_watchdog_not_running_before_start),
    NAVTEST_CASE(test_watchdog_kick_refused_before_start),
    NAVTEST_CASE(test_watchdog_rejects_zero_timeout),
    NAVTEST_CASE(test_watchdog_rejects_timeout_past_the_maximum),
    NAVTEST_CASE(test_watchdog_max_timeout_is_sane),
#if NAVHAL_CONFIG_DRV_WWDG
    NAVTEST_CASE(test_wwdg_not_running_before_start),
    NAVTEST_CASE(test_wwdg_kick_refused_before_start),
    NAVTEST_CASE(test_wwdg_rejects_bad_window),
    NAVTEST_CASE(test_wwdg_rejects_timeout_it_cannot_express),
#endif
};

const navtest_suite_t test_watchdog_suite = {
    .name = "WATCHDOG",
    .cases = watchdog_cases,
    .count = sizeof(watchdog_cases) / sizeof(watchdog_cases[0]),
    .between = NULL,
};

#endif /* NAVHAL_CONFIG_DRV_WATCHDOG */
