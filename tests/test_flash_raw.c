#include "test_flash_raw.h"
#include "core/cortex-m4/flash.h"
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

void test_hal_flash_save_rejects_null_value(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_flash_save(0x10, NULL, 4));
}

void test_hal_flash_read_rejects_null_pointers(void) {
  uint8_t buf = 0;
  uint8_t size = 0;
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_flash_read(0x10, NULL, &size));
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_flash_read(0x10, &buf, NULL));
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

static const navtest_case_t flash_cases[] = {
    NAVTEST_CASE(test_flash_storage_integration),
    NAVTEST_CASE(test_hal_flash_save_rejects_null_value),
    NAVTEST_CASE(test_hal_flash_read_rejects_null_pointers),
    NAVTEST_CASE(test_hal_flash_delete_then_read_returns_error),
    NAVTEST_CASE(test_hal_flash_needs_compaction_returns_bool),
    NAVTEST_CASE(test_hal_flash_erase_returns_ok),
};

const navtest_suite_t test_flash_suite = {
    .name = "FLASH RELIABILITY",
    .cases = flash_cases,
    .count = sizeof(flash_cases) / sizeof(flash_cases[0]),
    .between = NULL,
};
