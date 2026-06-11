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

#ifndef TEST_HOST_CONVERSION_H
#define TEST_HOST_CONVERSION_H

#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
void test_str_to_int_basic(void);
void test_str_to_int_negative(void);
void test_str_to_int_plus_sign(void);
void test_str_to_int_leading_whitespace(void);
void test_str_to_int_stops_at_non_digit(void);
void test_str_to_int_empty_string(void);

void test_str_to_float_basic(void);
void test_str_to_float_with_decimal(void);
void test_str_to_float_negative(void);

extern const navtest_suite_t test_conversion_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
