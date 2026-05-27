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

#ifndef TEST_FLASH_RAW_H
#define TEST_FLASH_RAW_H

#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
void test_flash_storage_integration(void);
void test_hal_flash_save_rejects_null_value(void);
void test_hal_flash_read_rejects_null_pointers(void);
void test_hal_flash_delete_then_read_returns_error(void);
void test_hal_flash_needs_compaction_returns_bool(void);
void test_hal_flash_erase_returns_ok(void);

extern const navtest_suite_t test_flash_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_FLASH_RAW_H
