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
 * @file test_flash_driver.c
 * @brief Deep host (SIL) tests for flash.c — the key/value store — against a
 *        simulated flash backing (host_mmio maps 0x08000000 and resets it to
 *        0xFF, i.e. erased). The half-word program writes into the backing, so
 *        the record format / find / CRC / update / delete logic is exercised
 *        end-to-end. FLASH_SR resets to 0 (BSY clear), so the BSY busy-waits
 *        return immediately.
 */

#include "host_mmio.h"
#include "navhal_port_flash.h"
#include "family/flash_reg.h"
#include "navtest/navtest.h"
#include <stdint.h>

void test_host_flash_save_read_roundtrip(void) {
  host_mmio_reset();
  uint8_t in[] = {0xAA, 0xBB, 0xCC, 0xDD};
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_flash_save(0x77, in, sizeof(in)));
  uint8_t out[8] = {0};
  uint8_t sz = 0;
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_flash_read(0x77, out, &sz));
  TEST_ASSERT_EQUAL_UINT32(sizeof(in), (uint32_t)sz);
  for (unsigned i = 0; i < sizeof(in); i++)
    TEST_ASSERT_EQUAL_UINT32(in[i], out[i]);
}

void test_host_flash_distinct_keys(void) {
  host_mmio_reset();
  uint8_t a[] = {1, 2};
  uint8_t b[] = {9, 8, 7};
  hal_flash_save(0x10, a, sizeof(a));
  hal_flash_save(0x20, b, sizeof(b));
  uint8_t out[8] = {0};
  uint8_t sz = 0;
  hal_flash_read(0x10, out, &sz);
  TEST_ASSERT_EQUAL_UINT32(2u, (uint32_t)sz);
  TEST_ASSERT_EQUAL_UINT32(2u, out[1]);
  hal_flash_read(0x20, out, &sz);
  TEST_ASSERT_EQUAL_UINT32(3u, (uint32_t)sz);
  TEST_ASSERT_EQUAL_UINT32(7u, out[2]);
}

void test_host_flash_update_returns_latest(void) {
  host_mmio_reset();
  uint8_t v1[] = {0x11, 0x11};
  uint8_t v2[] = {0x22, 0x22};
  hal_flash_save(0x55, v1, sizeof(v1));
  hal_flash_save(0x55, v2, sizeof(v2)); /* supersedes v1 */
  uint8_t out[4] = {0};
  uint8_t sz = 0;
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_flash_read(0x55, out, &sz));
  TEST_ASSERT_EQUAL_UINT32(0x22u, out[0]);
  TEST_ASSERT_EQUAL_UINT32(0x22u, out[1]);
}

void test_host_flash_delete_then_read_fails(void) {
  host_mmio_reset();
  uint8_t v[] = {0xEE, 0xFF};
  hal_flash_save(0x88, v, sizeof(v));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_flash_delete(0x88));
  uint8_t out[4] = {0};
  uint8_t sz = 0;
  TEST_ASSERT_TRUE(hal_flash_read(0x88, out, &sz) != HAL_OK);
}

void test_host_flash_read_missing_key_fails(void) {
  host_mmio_reset();
  uint8_t out[4] = {0};
  uint8_t sz = 7;
  TEST_ASSERT_TRUE(hal_flash_read(0x99, out, &sz) != HAL_OK);
  TEST_ASSERT_EQUAL_UINT32(0u, (uint32_t)sz);
}

void test_host_flash_save_rejects_bad_args(void) {
  uint8_t v[] = {1};
  TEST_ASSERT_TRUE(hal_flash_save(0x01, NULL, 4) != HAL_OK);
  TEST_ASSERT_TRUE(hal_flash_save(0x01, v, 0) != HAL_OK);
}

void test_host_flash_read_rejects_null(void) {
  uint8_t sz = 0, buf = 0;
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_flash_read(0x10, NULL, &sz));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_flash_read(0x10, &buf, NULL));
}

void test_host_flash_needs_compaction_false_when_room(void) {
  host_mmio_reset();
  TEST_ASSERT_FALSE(hal_flash_needs_compaction());
}

NAVTEST_CASE_DECL(test_host_flash_save_read_roundtrip);
NAVTEST_CASE_DECL(test_host_flash_distinct_keys);
NAVTEST_CASE_DECL(test_host_flash_update_returns_latest);
NAVTEST_CASE_DECL(test_host_flash_delete_then_read_fails);
NAVTEST_CASE_DECL(test_host_flash_read_missing_key_fails);
NAVTEST_CASE_DECL(test_host_flash_save_rejects_bad_args);
NAVTEST_CASE_DECL(test_host_flash_read_rejects_null);
NAVTEST_CASE_DECL(test_host_flash_needs_compaction_false_when_room);

static const navtest_case_t flash_driver_cases[] = {
    NAVTEST_CASE(test_host_flash_save_read_roundtrip),
    NAVTEST_CASE(test_host_flash_distinct_keys),
    NAVTEST_CASE(test_host_flash_update_returns_latest),
    NAVTEST_CASE(test_host_flash_delete_then_read_fails),
    NAVTEST_CASE(test_host_flash_read_missing_key_fails),
    NAVTEST_CASE(test_host_flash_save_rejects_bad_args),
    NAVTEST_CASE(test_host_flash_read_rejects_null),
    NAVTEST_CASE(test_host_flash_needs_compaction_false_when_room),
};

const navtest_suite_t test_flash_driver_suite = {
    .name = "FLASH DRIVER (host)",
    .cases = flash_driver_cases,
    .count = sizeof(flash_driver_cases) / sizeof(flash_driver_cases[0]),
    .between = NULL,
};
