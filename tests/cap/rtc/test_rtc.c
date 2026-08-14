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
 * @file test_rtc.c
 * @brief On-target tests for the RTC calendar and backup registers.
 *
 * Device-free: the calendar runs from the internal RC when the board has no
 * crystal, so these pass on a bare board. They do overwrite whatever time the
 * RTC was holding, which is the only way to test setting it.
 */

#include "test_rtc.h"
#include "common/hal_features.h"
#include "navtest/navtest.h"
#include "navtest/navtest_pil.h"
#include <stdint.h>

#if NAVHAL_CONFIG_DRV_RTC
#include "common/hal_rtc.h"

/* Bound on the wait for the calendar's next second. The RTC ticks at 1 Hz off a
 * ~32 kHz oscillator, so this only has to outlast one second of CPU spinning at
 * whatever clock the test build happens to run at. */
#define SECOND_TICK_SPINS 40000000UL

static const hal_rtc_datetime_t reference = {
    .year = 2025,
    .month = 3,
    .day = 9,
    .weekday = HAL_RTC_SUNDAY,
    .hour = 23,
    .minute = 59,
    .second = 5,
};

void test_rtc_init_returns_ok(void) {
  NAVTEST_SKIP_ON_PIL();
  TEST_ASSERT_EQUAL_UINT32(HAL_OK, hal_rtc_init(NULL)); /* NULL selects AUTO */
}

void test_rtc_clock_is_running(void) {
  NAVTEST_SKIP_ON_PIL();
  hal_rtc_init(NULL);
  hal_rtc_clock_t clock = hal_rtc_get_clock();
  /* Either oscillator is a pass — which one depends on the board. */
  TEST_ASSERT_TRUE(clock == HAL_RTC_CLOCK_LSE || clock == HAL_RTC_CLOCK_LSI);
}

void test_rtc_set_then_get_round_trips(void) {
  NAVTEST_SKIP_ON_PIL();
  hal_rtc_init(NULL);
  TEST_ASSERT_EQUAL_UINT32(HAL_OK, hal_rtc_set_datetime(&reference));

  hal_rtc_datetime_t got = {0};
  TEST_ASSERT_EQUAL_UINT32(HAL_OK, hal_rtc_get_datetime(&got));

  TEST_ASSERT_EQUAL_UINT32(reference.year, got.year);
  TEST_ASSERT_EQUAL_UINT32(reference.month, got.month);
  TEST_ASSERT_EQUAL_UINT32(reference.day, got.day);
  TEST_ASSERT_EQUAL_UINT32(reference.weekday, got.weekday);
  TEST_ASSERT_EQUAL_UINT32(reference.hour, got.hour);
  TEST_ASSERT_EQUAL_UINT32(reference.minute, got.minute);
  /* Seconds may have advanced between the write and the read. */
  TEST_ASSERT_TRUE(got.second >= reference.second);
}

void test_rtc_counts_seconds(void) {
  NAVTEST_SKIP_ON_PIL();
  hal_rtc_init(NULL);
  hal_rtc_set_datetime(&reference);

  hal_rtc_datetime_t first = {0}, later = {0};
  hal_rtc_get_datetime(&first);

  uint32_t spins = SECOND_TICK_SPINS;
  do {
    hal_rtc_get_datetime(&later);
  } while (later.second == first.second && --spins);

  /* A calendar that never leaves its starting second is configured but not
   * clocked — the failure mode when the oscillator behind it is not running. */
  TEST_ASSERT_TRUE(spins > 0);
  TEST_ASSERT_TRUE(later.second != first.second);
}

void test_rtc_is_set_after_setting(void) {
  NAVTEST_SKIP_ON_PIL();
  hal_rtc_init(NULL);
  hal_rtc_set_datetime(&reference);
  TEST_ASSERT_TRUE(hal_rtc_is_set());
}

void test_rtc_set_rejects_out_of_range(void) {
  NAVTEST_SKIP_ON_PIL();
  hal_rtc_init(NULL);

  hal_rtc_datetime_t bad = reference;
  bad.month = 13;
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG, hal_rtc_set_datetime(&bad));

  bad = reference;
  bad.hour = 24;
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG, hal_rtc_set_datetime(&bad));

  bad = reference;
  bad.year = 1999;
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG, hal_rtc_set_datetime(&bad));

  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG, hal_rtc_set_datetime(NULL));
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG, hal_rtc_get_datetime(NULL));
}

void test_rtc_backup_round_trips(void) {
  NAVTEST_SKIP_ON_PIL();
  hal_rtc_init(NULL);

  uint32_t value = 0;
  TEST_ASSERT_EQUAL_UINT32(HAL_OK, hal_rtc_backup_write(0, 0xC0FFEE01U));
  TEST_ASSERT_EQUAL_UINT32(HAL_OK,
                         hal_rtc_backup_write(HAL_RTC_BACKUP_COUNT - 1, 0x5A5AU));

  TEST_ASSERT_EQUAL_UINT32(HAL_OK, hal_rtc_backup_read(0, &value));
  TEST_ASSERT_EQUAL_UINT32(0xC0FFEE01U, value);
  TEST_ASSERT_EQUAL_UINT32(HAL_OK,
                         hal_rtc_backup_read(HAL_RTC_BACKUP_COUNT - 1, &value));
  TEST_ASSERT_EQUAL_UINT32(0x5A5AU, value);
}

void test_rtc_backup_rejects_bad_index(void) {
  NAVTEST_SKIP_ON_PIL();
  hal_rtc_init(NULL);

  uint32_t value = 0;
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG,
                         hal_rtc_backup_write(HAL_RTC_BACKUP_COUNT, 1));
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG,
                         hal_rtc_backup_read(HAL_RTC_BACKUP_COUNT, &value));
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG, hal_rtc_backup_read(0, NULL));
}

NAVTEST_CASE_DECL(test_rtc_init_returns_ok);
NAVTEST_CASE_DECL(test_rtc_clock_is_running);
NAVTEST_CASE_DECL(test_rtc_set_then_get_round_trips);
NAVTEST_CASE_DECL(test_rtc_counts_seconds);
NAVTEST_CASE_DECL(test_rtc_is_set_after_setting);
NAVTEST_CASE_DECL(test_rtc_set_rejects_out_of_range);
NAVTEST_CASE_DECL(test_rtc_backup_round_trips);
NAVTEST_CASE_DECL(test_rtc_backup_rejects_bad_index);

static const navtest_case_t rtc_cases[] = {
    NAVTEST_CASE(test_rtc_init_returns_ok),
    NAVTEST_CASE(test_rtc_clock_is_running),
    NAVTEST_CASE(test_rtc_set_then_get_round_trips),
    NAVTEST_CASE(test_rtc_counts_seconds),
    NAVTEST_CASE(test_rtc_is_set_after_setting),
    NAVTEST_CASE(test_rtc_set_rejects_out_of_range),
    NAVTEST_CASE(test_rtc_backup_round_trips),
    NAVTEST_CASE(test_rtc_backup_rejects_bad_index),
};

const navtest_suite_t test_rtc_suite = {
    .name = "RTC",
    .cases = rtc_cases,
    .count = sizeof(rtc_cases) / sizeof(rtc_cases[0]),
    .between = NULL,
};

#endif /* NAVHAL_CONFIG_DRV_RTC */
