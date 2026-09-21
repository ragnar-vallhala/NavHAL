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

#ifndef TEST_HOST_MPU_H
#define TEST_HOST_MPU_H

#include "navtest/navtest.h"

#ifdef __cplusplus
extern "C" {
#endif

void test_mpu_absent_reports_not_supported(void);
void test_mpu_present_reports_region_count(void);
void test_mpu_encode_normal_wb_vector(void);
void test_mpu_encode_device_vector(void);
void test_mpu_encode_rejects_bad_args(void);
void test_mpu_enable_disable_ctrl(void);
void test_mpu_configure_region_writes_regs(void);
void test_mpu_disable_region_clears_enable(void);
void test_mpu_apply_writes_each_pair(void);

extern const navtest_suite_t test_mpu_suite;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* TEST_HOST_MPU_H */
