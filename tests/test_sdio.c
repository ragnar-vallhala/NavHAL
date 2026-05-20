/**
 * @file tests/test_sdio.c
 * @brief Standardized hal_sdio_* API smoke tests.
 *
 * SDIO sits behind NAVHAL_HAS_SDIO. The actual card is rarely connected
 * on a bare Nucleo, so the tests cover argument validation, the typed
 * error-code surface, and ensure that mutating helpers don't crash on
 * smoke-test inputs.
 */

#define CORTEX_M4
#include "test_sdio.h"

#if NAVHAL_HAS_SDIO

#include "core/cortex-m4/sdio.h"
#include "navtest/navtest.h"

void test_hal_sdio_init_rejects_null_config(void) {
  /* The contract is hal_sdio_error_t; HAL_SDIO_ERROR is the generic
   * rejection code. */
  TEST_ASSERT_TRUE(hal_sdio_init(NULL) != HAL_SDIO_OK);
}

void test_hal_sdio_read_block_rejects_null_buffer(void) {
  TEST_ASSERT_TRUE(hal_sdio_read_block(0, NULL) != HAL_SDIO_OK);
}

void test_hal_sdio_write_block_rejects_null_buffer(void) {
  TEST_ASSERT_TRUE(hal_sdio_write_block(0, NULL) != HAL_SDIO_OK);
}

void test_hal_sdio_get_sector_count_returns_value(void) {
  /* Without a card the sector count may be 0 — what matters is the call
   * returns and doesn't fault. */
  uint32_t n = hal_sdio_get_sector_count();
  (void)n;
  TEST_ASSERT_TRUE(1);
}

static volatile uint32_t s_sdio_cb_hits = 0;
static void sdio_test_cb(hal_sdio_error_t err) {
  (void)err;
  s_sdio_cb_hits++;
}

void test_hal_sdio_set_callback_smoke(void) {
  s_sdio_cb_hits = 0;
  hal_sdio_set_callback(sdio_test_cb);
  hal_sdio_set_callback(NULL); /* re-clear */
  TEST_ASSERT_TRUE(1);
}

static const navtest_case_t sdio_cases[] = {
    NAVTEST_CASE(test_hal_sdio_init_rejects_null_config),
    NAVTEST_CASE(test_hal_sdio_read_block_rejects_null_buffer),
    NAVTEST_CASE(test_hal_sdio_write_block_rejects_null_buffer),
    NAVTEST_CASE(test_hal_sdio_get_sector_count_returns_value),
    NAVTEST_CASE(test_hal_sdio_set_callback_smoke),
};

const navtest_suite_t test_sdio_suite = {
    .name = "SDIO",
    .cases = sdio_cases,
    .count = sizeof(sdio_cases) / sizeof(sdio_cases[0]),
    .between = NULL,
};

#endif /* NAVHAL_HAS_SDIO */
