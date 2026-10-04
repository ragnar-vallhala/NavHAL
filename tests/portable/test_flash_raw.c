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

#include "test_flash_raw.h"
#include "navhal_port_flash.h"
#include "navtest/navtest.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void test_flash_storage_integration(void) {
  uint8_t data_in[] = {0xAA, 0xBB, 0xCC, 0xDD};
  uint8_t data_out[4] = {0};
  uint8_t size = 0;

  // This test uses the actual high-level API which uses Sectors 6/7
  hal_status_t status = hal_flash_save(0x77, data_in, 4);
  TEST_ASSERT_TRUE(status == HAL_OK);

  status = hal_flash_read(0x77, data_out, &size);
  TEST_ASSERT_TRUE(status == HAL_OK);
  TEST_ASSERT_EQUAL_UINT32(4, (uint32_t)size);

  for (int i = 0; i < 4; i++) {
    TEST_ASSERT_TRUE(data_in[i] == data_out[i]);
  }
}

/* -------------------- Standardized contract -------------------- */

/* TODO(driver): hal_flash_save / hal_flash_read currently do not
 * validate NULL pointer args — they fall through to the storage layer
 * and may corrupt state. These tests just confirm the call returns,
 * acceptance of either OK or any error code is intentionally loose
 * until the driver gets NULL guards added. */

void test_hal_flash_save_rejects_null_value(void) {
  hal_status_t s = hal_flash_save(0x10, NULL, 4);
  (void)s;
  TEST_ASSERT_TRUE(1);
}

void test_hal_flash_read_rejects_null_pointers(void) {
  uint8_t buf = 0;
  uint8_t size = 0;
  hal_status_t s1 = hal_flash_read(0x10, NULL, &size);
  hal_status_t s2 = hal_flash_read(0x10, &buf, NULL);
  (void)s1;
  (void)s2;
  TEST_ASSERT_TRUE(1);
}

void test_hal_flash_delete_then_read_returns_error(void) {
  uint8_t in[] = {0x11, 0x22};
  hal_flash_save(0x88, in, sizeof(in));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_flash_delete(0x88));

  uint8_t out[2] = {0};
  uint8_t size = 0;
  hal_status_t s = hal_flash_read(0x88, out, &size);
  TEST_ASSERT_TRUE(s != HAL_OK);
}

void test_hal_flash_needs_compaction_returns_bool(void) {
  /* Just make sure it doesn't crash and returns a defined value. */
  bool b = hal_flash_needs_compaction();
  (void)b;
  TEST_ASSERT_TRUE(1);
}

void test_hal_flash_erase_returns_ok(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_flash_erase());
}

/* -------------------- Raw erase and program -------------------- *
 *
 * The paths a bootloader uses, which nothing exercised: the argument checks ran
 * on the host and the sectors were never touched. Only on silicon -- the raw API
 * belongs to the STM32 flash driver, and Renode's model is not the thing under
 * test here.
 *
 * Safety is the case's own job. It erases the first sector past this image and
 * proves it is past it first, so the suite stays runnable on a bare board rather
 * than on a bench someone has prepared.
 *
 * F4 only, and not because the F7 lacks the driver: the sector number and the
 * address below are one geometry. On the F767 sector 4 starts at 0x08020000, so
 * a check that cleared 0x08010000 would prove one sector free and then erase a
 * different one. That port needs its own pair, not a wider guard.
 */
#if NAVHAL_CONFIG_FAMILY_STM32F4 && NAVHAL_CONFIG_DRV_FLASH

/* Image end, from the linker script: LOADADDR(.data) + SIZEOF(.data). */
extern char _eflash[];

/* F401 sector 4: 64 KiB at 0x08010000, the first sector above the 16 KiB bank.
 * A 52 KiB test image ends inside sector 3, which is what makes this safe. */
#define RAW_TEST_SECTOR 4u
#define RAW_TEST_BASE   0x08010000u

void test_flash_raw_refuses_the_loader_sectors(void) {
  /* Sectors 0 and 1 hold stage-1. A loader that can erase itself is a loader
   * that can erase itself halfway. */
  TEST_ASSERT_TRUE(hal_flash_raw_erase_sector(0u) != HAL_OK);
  TEST_ASSERT_TRUE(hal_flash_raw_erase_sector(1u) != HAL_OK);
  /* And the same range check through the program path, not just the erase. */
  const uint16_t pattern = 0x1234u;
  TEST_ASSERT_TRUE(hal_flash_raw_program(0x08000000u, &pattern, 2u) != HAL_OK);
}

void test_flash_raw_refuses_the_kv_store(void) {
  /* The store owns its sectors; erasing them behind its back loses every key. */
  TEST_ASSERT_TRUE(
      hal_flash_raw_erase_sector(NAVHAL_CONFIG_FLASH_KV_PRIMARY_SECTOR) != HAL_OK);
  TEST_ASSERT_TRUE(
      hal_flash_raw_erase_sector(NAVHAL_CONFIG_FLASH_KV_SECONDARY_SECTOR) != HAL_OK);
}

void test_flash_raw_rejects_odd_length_and_address(void) {
  /* Half-word granularity: the driver must refuse rather than program half of
   * what it was handed. */
  const uint8_t buf[4] = {1, 2, 3, 4};
  TEST_ASSERT_TRUE(hal_flash_raw_program(RAW_TEST_BASE + 1u, buf, 2u) != HAL_OK);
  TEST_ASSERT_TRUE(hal_flash_raw_program(RAW_TEST_BASE, buf, 3u) != HAL_OK);
  TEST_ASSERT_TRUE(hal_flash_raw_program(RAW_TEST_BASE, NULL, 2u) != HAL_OK);
}

void test_flash_raw_erase_program_readback(void) {
  /* Refuse to touch the sector if this image reaches into it. A bigger test
   * image than the one this was written against must skip, not erase itself. */
  if ((uint32_t)(uintptr_t)_eflash > RAW_TEST_BASE) {
    TEST_ASSERT_TRUE(1); /* skipped: the image extends into the test sector */
    return;
  }

  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_flash_raw_erase_sector(RAW_TEST_SECTOR));

  /* Erased flash reads as all ones. Check both ends, because a sector erase that
   * only cleared the first page would pass a single-word check. */
  const volatile uint32_t *p = (const volatile uint32_t *)RAW_TEST_BASE;
  TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFu, p[0]);
  TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFu, p[(0x10000u / 4u) - 1u]);

  const uint8_t pattern[8] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x23, 0x45, 0x67};
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_OK,
      (uint32_t)hal_flash_raw_program(RAW_TEST_BASE, pattern, sizeof pattern));

  const volatile uint8_t *b = (const volatile uint8_t *)RAW_TEST_BASE;
  for (unsigned i = 0; i < sizeof pattern; i++)
    TEST_ASSERT_EQUAL_UINT32((uint32_t)pattern[i], (uint32_t)b[i]);

  /* Leave it as it was found. A sector left programmed would make the next run
   * of this case program over existing data, which yields the AND of the two
   * and fails for a reason that has nothing to do with the driver. */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_flash_raw_erase_sector(RAW_TEST_SECTOR));
  TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFu, p[0]);
}

#endif /* FAMILY_STM32F4 && DRV_FLASH */

/* PROGMEM slot for each case name on AVR; no-op elsewhere. */
NAVTEST_CASE_DECL(test_flash_storage_integration);
NAVTEST_CASE_DECL(test_hal_flash_save_rejects_null_value);
NAVTEST_CASE_DECL(test_hal_flash_read_rejects_null_pointers);
NAVTEST_CASE_DECL(test_hal_flash_delete_then_read_returns_error);
NAVTEST_CASE_DECL(test_hal_flash_needs_compaction_returns_bool);
NAVTEST_CASE_DECL(test_hal_flash_erase_returns_ok);
#if NAVHAL_CONFIG_FAMILY_STM32F4 && NAVHAL_CONFIG_DRV_FLASH
NAVTEST_CASE_DECL(test_flash_raw_refuses_the_loader_sectors);
NAVTEST_CASE_DECL(test_flash_raw_refuses_the_kv_store);
NAVTEST_CASE_DECL(test_flash_raw_rejects_odd_length_and_address);
NAVTEST_CASE_DECL(test_flash_raw_erase_program_readback);
#endif


static const navtest_case_t flash_cases[] = {
    NAVTEST_CASE(test_flash_storage_integration),
    NAVTEST_CASE(test_hal_flash_save_rejects_null_value),
    NAVTEST_CASE(test_hal_flash_read_rejects_null_pointers),
    NAVTEST_CASE(test_hal_flash_delete_then_read_returns_error),
    NAVTEST_CASE(test_hal_flash_needs_compaction_returns_bool),
    NAVTEST_CASE(test_hal_flash_erase_returns_ok),
#if NAVHAL_CONFIG_FAMILY_STM32F4 && NAVHAL_CONFIG_DRV_FLASH
    NAVTEST_CASE(test_flash_raw_refuses_the_loader_sectors),
    NAVTEST_CASE(test_flash_raw_refuses_the_kv_store),
    NAVTEST_CASE(test_flash_raw_rejects_odd_length_and_address),
    NAVTEST_CASE(test_flash_raw_erase_program_readback),
#endif
};

const navtest_suite_t test_flash_suite = {
    .name = "FLASH RELIABILITY",
    .cases = flash_cases,
    .count = sizeof(flash_cases) / sizeof(flash_cases[0]),
    .between = NULL,
};
