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
 * @file tests/test_sdio.h
 * @brief Standardized hal_sdio_* API smoke tests.
 *
 * The SDIO driver speaks ::hal_sdio_error_t rather than ::hal_status_t
 * (per its module note); these tests cover the typed-id surface and the
 * error-code contract for the common-failure paths.
 */

#ifndef TEST_SDIO_H
#define TEST_SDIO_H

#include "common/hal_features.h"
#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
#if NAVHAL_HAS_SDIO

void test_hal_sdio_init_rejects_null_config(void);
void test_hal_sdio_read_block_rejects_null_buffer(void);
void test_hal_sdio_write_block_rejects_null_buffer(void);
void test_hal_sdio_get_sector_count_returns_value(void);
void test_hal_sdio_set_callback_smoke(void);
void test_hal_sdio_block_roundtrip_pil(void);

extern const navtest_suite_t test_sdio_suite;

#endif /* NAVHAL_HAS_SDIO */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_SDIO_H
