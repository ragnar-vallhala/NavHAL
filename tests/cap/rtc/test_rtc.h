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

#ifndef TEST_RTC_H
#define TEST_RTC_H

#include "common/hal_features.h"
#include "navtest/navtest.h"

#ifdef __cplusplus
extern "C" {
#endif
#if NAVHAL_CONFIG_DRV_RTC

void test_rtc_init_returns_ok(void);
void test_rtc_clock_is_running(void);
void test_rtc_set_then_get_round_trips(void);
void test_rtc_counts_seconds(void);
void test_rtc_is_set_after_setting(void);
void test_rtc_set_rejects_out_of_range(void);
void test_rtc_backup_round_trips(void);
void test_rtc_backup_rejects_bad_index(void);

extern const navtest_suite_t test_rtc_suite;

#endif /* NAVHAL_CONFIG_DRV_RTC */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_RTC_H
