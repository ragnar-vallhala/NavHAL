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
 * @file tests/test_timebase.h
 * @brief Standardized hal_timebase_* API tests.
 *
 * The timebase replaces the SysTick-specific naming from M2. The
 * timebase concept generalizes to non-Cortex MCUs (M6 / AVR will use a
 * general-purpose timer for the same role).
 */

#ifndef TEST_TIMEBASE_H
#define TEST_TIMEBASE_H

#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
void test_hal_timebase_init_returns_ok(void);
void test_hal_timebase_tick_increments(void);
void test_hal_timebase_get_tick_duration_us_matches_init(void);
void test_hal_timebase_get_millis_is_monotonic(void);
void test_hal_timebase_get_micros_is_monotonic(void);
void test_hal_timebase_get_reload_value_nonzero(void);
void test_hal_timebase_set_callback_rejects_null(void);
void test_hal_timebase_returns_uint32(void);

extern const navtest_suite_t test_timebase_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_TIMEBASE_H
