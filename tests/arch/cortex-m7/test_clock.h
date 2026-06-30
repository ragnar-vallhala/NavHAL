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

#ifndef TEST_CLOCK_H
#define TEST_CLOCK_H

#include "navtest/navtest.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
// -------------------- Clock Initialization --------------------
void test_hal_clock_init_hsi(void);
void test_hal_clock_init_pll(void);

// -------------------- SYSCLK --------------------
void test_hal_clock_get_sysclk_returns_correct_value_hsi(void);
void test_hal_clock_get_sysclk_returns_correct_value_pll(void);

// -------------------- AHB / APB --------------------
void test_hal_clock_get_ahbclk_returns_correct_value(void);
void test_hal_clock_get_apb1clk_returns_correct_value(void);
void test_hal_clock_get_apb2clk_returns_correct_value(void);

// -------------------- Status-return contract --------------------
void test_hal_clock_init_returns_ok_for_hsi(void);
void test_hal_clock_init_rejects_null_cfg(void);
void test_hal_clock_init_pll_rejects_null_pll_cfg(void);

extern const navtest_suite_t test_clock_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_CLOCK_H
