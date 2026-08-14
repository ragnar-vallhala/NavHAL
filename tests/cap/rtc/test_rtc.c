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

/* Spin until `flag` or the bound runs out; the bound only has to outlast a
 * couple of RTC seconds at whatever clock the test build runs at. */
#define POLL_UNTIL(flag)                                                       \
  ({                                                                           \
    uint32_t _spins = SECOND_TICK_SPINS * 3u;                                  \
    while (!(flag) && --_spins) {                                              \
    }                                                                          \
    _spins != 0u;                                                              \
  })

static volatile uint32_t alarm_hits;
static void on_alarm(void) { alarm_hits++; }

void test_rtc_wakeup_fires_and_repeats(void) {
  NAVTEST_SKIP_ON_PIL();
  hal_rtc_init(NULL);

  /* No callback: the flag still sets, which is the polling path. */
  TEST_ASSERT_EQUAL_UINT32(HAL_OK, hal_rtc_set_wakeup(1000u, NULL));
  TEST_ASSERT_TRUE(POLL_UNTIL(hal_rtc_wakeup_fired()));
  /* It reloads itself, so a second period follows without re-arming. */
  TEST_ASSERT_TRUE(POLL_UNTIL(hal_rtc_wakeup_fired()));
  hal_rtc_cancel_wakeup();
}

void test_rtc_cancel_wakeup_stops_it(void) {
  NAVTEST_SKIP_ON_PIL();
  hal_rtc_init(NULL);
  hal_rtc_set_wakeup(1000u, NULL);
  TEST_ASSERT_EQUAL_UINT32(HAL_OK, hal_rtc_cancel_wakeup());
  (void)hal_rtc_wakeup_fired(); /* drop anything already pending */

  uint32_t spins = SECOND_TICK_SPINS * 3u;
  while (--spins) {
  }
  TEST_ASSERT_FALSE(hal_rtc_wakeup_fired());
}

void test_rtc_alarm_fires_on_match(void) {
  NAVTEST_SKIP_ON_PIL();
  hal_rtc_init(NULL);
  hal_rtc_set_datetime(&reference);

  hal_rtc_datetime_t now;
  hal_rtc_get_datetime(&now);

  /* Two seconds out, matching the second alone — far enough ahead that the
   * calendar cannot have passed it before the alarm is armed. */
  hal_rtc_alarm_config_t cfg = {
      .second = (uint8_t)((now.second + 2u) % 60u),
      .match = HAL_RTC_MATCH_SECOND,
  };
  TEST_ASSERT_EQUAL_UINT32(HAL_OK,
                           hal_rtc_set_alarm(HAL_RTC_ALARM_A, &cfg, NULL));
  TEST_ASSERT_TRUE(POLL_UNTIL(hal_rtc_alarm_fired(HAL_RTC_ALARM_A)));
  hal_rtc_cancel_alarm(HAL_RTC_ALARM_A);
}

void test_rtc_alarm_callback_runs(void) {
  NAVTEST_SKIP_ON_PIL();
  hal_rtc_init(NULL);
  hal_rtc_set_datetime(&reference);

  hal_rtc_datetime_t now;
  hal_rtc_get_datetime(&now);
  alarm_hits = 0;

  /* With a callback the whole path is under test: RTC flag, EXTI line, NVIC. */
  hal_rtc_alarm_config_t cfg = {
      .second = (uint8_t)((now.second + 2u) % 60u),
      .match = HAL_RTC_MATCH_SECOND,
  };
  TEST_ASSERT_EQUAL_UINT32(HAL_OK,
                           hal_rtc_set_alarm(HAL_RTC_ALARM_B, &cfg, on_alarm));
  TEST_ASSERT_TRUE(POLL_UNTIL(alarm_hits > 0u));
  hal_rtc_cancel_alarm(HAL_RTC_ALARM_B);
}

void test_rtc_alarm_rejects_bad_args(void) {
  NAVTEST_SKIP_ON_PIL();
  hal_rtc_init(NULL);

  hal_rtc_alarm_config_t cfg = {.second = 0, .match = HAL_RTC_MATCH_SECOND};
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG,
                           hal_rtc_set_alarm(HAL_RTC_ALARM_A, NULL, NULL));
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG,
                           hal_rtc_set_alarm((hal_rtc_alarm_t)7, &cfg, NULL));

  cfg.hour = 24;
  cfg.match = HAL_RTC_MATCH_HOUR;
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG,
                           hal_rtc_set_alarm(HAL_RTC_ALARM_A, &cfg, NULL));

  /* A day of 0 is fine as long as the alarm does not compare the day. */
  hal_rtc_alarm_config_t unmatched = {.second = 5,
                                      .match = HAL_RTC_MATCH_SECOND};
  TEST_ASSERT_EQUAL_UINT32(HAL_OK,
                           hal_rtc_set_alarm(HAL_RTC_ALARM_A, &unmatched, NULL));
  hal_rtc_cancel_alarm(HAL_RTC_ALARM_A);
}

void test_rtc_wakeup_rejects_bad_period(void) {
  NAVTEST_SKIP_ON_PIL();
  hal_rtc_init(NULL);
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG, hal_rtc_set_wakeup(0u, NULL));
  TEST_ASSERT_EQUAL_UINT32(HAL_ERR_INVALID_ARG,
                           hal_rtc_set_wakeup(65536001u, NULL));
}

NAVTEST_CASE_DECL(test_rtc_init_returns_ok);
NAVTEST_CASE_DECL(test_rtc_clock_is_running);
NAVTEST_CASE_DECL(test_rtc_set_then_get_round_trips);
NAVTEST_CASE_DECL(test_rtc_counts_seconds);
NAVTEST_CASE_DECL(test_rtc_is_set_after_setting);
NAVTEST_CASE_DECL(test_rtc_set_rejects_out_of_range);
NAVTEST_CASE_DECL(test_rtc_backup_round_trips);
NAVTEST_CASE_DECL(test_rtc_backup_rejects_bad_index);
NAVTEST_CASE_DECL(test_rtc_wakeup_fires_and_repeats);
NAVTEST_CASE_DECL(test_rtc_cancel_wakeup_stops_it);
NAVTEST_CASE_DECL(test_rtc_alarm_fires_on_match);
NAVTEST_CASE_DECL(test_rtc_alarm_callback_runs);
NAVTEST_CASE_DECL(test_rtc_alarm_rejects_bad_args);
NAVTEST_CASE_DECL(test_rtc_wakeup_rejects_bad_period);

static const navtest_case_t rtc_cases[] = {
    NAVTEST_CASE(test_rtc_init_returns_ok),
    NAVTEST_CASE(test_rtc_clock_is_running),
    NAVTEST_CASE(test_rtc_set_then_get_round_trips),
    NAVTEST_CASE(test_rtc_counts_seconds),
    NAVTEST_CASE(test_rtc_is_set_after_setting),
    NAVTEST_CASE(test_rtc_set_rejects_out_of_range),
    NAVTEST_CASE(test_rtc_backup_round_trips),
    NAVTEST_CASE(test_rtc_backup_rejects_bad_index),
    NAVTEST_CASE(test_rtc_wakeup_fires_and_repeats),
    NAVTEST_CASE(test_rtc_cancel_wakeup_stops_it),
    NAVTEST_CASE(test_rtc_alarm_fires_on_match),
    NAVTEST_CASE(test_rtc_alarm_callback_runs),
    NAVTEST_CASE(test_rtc_alarm_rejects_bad_args),
    NAVTEST_CASE(test_rtc_wakeup_rejects_bad_period),
};

const navtest_suite_t test_rtc_suite = {
    .name = "RTC",
    .cases = rtc_cases,
    .count = sizeof(rtc_cases) / sizeof(rtc_cases[0]),
    .between = NULL,
};

#endif /* NAVHAL_CONFIG_DRV_RTC */
