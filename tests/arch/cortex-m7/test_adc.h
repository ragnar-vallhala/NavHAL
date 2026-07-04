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

#ifndef TEST_ADC_H
#define TEST_ADC_H

#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
void test_adc_init_returns_ok(void);
void test_adc_init_rejects_bad_unit(void);
void test_adc_clock_enabled_after_init(void);
void test_adc_read_rejects_null(void);
void test_adc_read_completes_and_in_range(void);

extern const navtest_suite_t test_adc_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_ADC_H
