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

#ifndef TEST_WATCHDOG_H
#define TEST_WATCHDOG_H

#include "common/hal_features.h"
#include "navtest/navtest.h"

#ifdef __cplusplus
extern "C" {
#endif
#if NAVHAL_CONFIG_DRV_WATCHDOG

void test_watchdog_not_running_before_start(void);
void test_watchdog_kick_refused_before_start(void);
void test_watchdog_rejects_zero_timeout(void);
void test_watchdog_rejects_timeout_past_the_maximum(void);
void test_watchdog_max_timeout_is_sane(void);
#if NAVHAL_CONFIG_DRV_WWDG
void test_wwdg_not_running_before_start(void);
void test_wwdg_kick_refused_before_start(void);
void test_wwdg_rejects_bad_window(void);
void test_wwdg_rejects_timeout_it_cannot_express(void);
#endif

extern const navtest_suite_t test_watchdog_suite;

#endif /* NAVHAL_CONFIG_DRV_WATCHDOG */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_WATCHDOG_H
