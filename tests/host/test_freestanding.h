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

#ifndef TEST_HOST_FREESTANDING_H
#define TEST_HOST_FREESTANDING_H

#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
void test_freestanding_memcpy_every_size_and_offset(void);
void test_freestanding_memmove_every_overlap(void);
void test_freestanding_memset_every_size_and_offset(void);
void test_freestanding_memcmp_sign_at_every_byte(void);
void test_freestanding_large_random(void);

extern const navtest_suite_t test_freestanding_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
