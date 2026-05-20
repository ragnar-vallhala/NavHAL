#include "test_flash_raw.h"
#include "core/cortex-m4/flash.h"
#include "navtest/navtest.h"
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

static const navtest_case_t flash_cases[] = {
    NAVTEST_CASE(test_flash_storage_integration),
};

const navtest_suite_t test_flash_suite = {
    .name = "FLASH RELIABILITY",
    .cases = flash_cases,
    .count = sizeof(flash_cases) / sizeof(flash_cases[0]),
    .between = NULL,
};
