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

#ifndef TEST_FPU_ACCEL_H
#define TEST_FPU_ACCEL_H

#include "common/hal_features.h"
#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
#if NAVHAL_HAS_FPU

void test_fpu_basic_arithmetic(void);
void test_fpu_benchmark_cycles(void);
void test_hal_fpu_enable_returns_ok(void);

extern const navtest_suite_t test_fpu_suite;

#endif /* NAVHAL_HAS_FPU */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_FPU_ACCEL_H
