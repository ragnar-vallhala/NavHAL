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

#ifndef TEST_DWT_H
#define TEST_DWT_H

#include "common/hal_features.h"
#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
#if NAVHAL_HAS_CYCLE_COUNTER

void test_dwt_init_enables_counters(void);
void test_dwt_get_cycles_increments(void);
void test_dwt_reset_cycles_zeros_counter(void);
void test_dwt_delay_cycles_elapses_time(void);
void test_hal_cycle_counter_init_returns_ok(void);
void test_hal_cycle_counter_reset_returns_ok(void);

extern const navtest_suite_t test_dwt_suite;

#endif /* NAVHAL_HAS_CYCLE_COUNTER */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_DWT_H
