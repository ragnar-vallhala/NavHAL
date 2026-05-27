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
 * @file tests/test_crc.h
 * @brief CRC test function declarations for NavTest.
 */

#ifndef TEST_CRC_H
#define TEST_CRC_H

#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
void test_crc_empty_returns_init(void);
void test_crc_single_byte(void);
void test_crc_known_vector(void);
void test_crc_accumulate_matches_compute(void);
void test_crc_reset_restores_init(void);
void test_hal_crc_init_rejects_null_config(void);
void test_hal_crc_compute_mpeg2_reference_vector(void);

extern const navtest_suite_t test_crc_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* TEST_CRC_H */
