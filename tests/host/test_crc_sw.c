/**
 * @file tests/host/test_crc_sw.c
 * @brief Host-runnable tests for the software CRC-32/MPEG-2 path.
 *
 * The driver under src/vendor/stm32/crc/crc.c provides a hardware path
 * (when _CRC_HW_ENABLED is defined) and a software lookup-table path
 * otherwise. The host build does not define _CRC_HW_ENABLED, so the same
 * source compiles down to the pure-C software implementation — exercised
 * here against the standardized hal_crc_* API.
 */

#include "navhal_port_crc.h"
#include "test_crc_sw.h"

void test_crc_sw_init_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_crc_init(NULL));
}

static void init_default(void) {
  hal_crc_config_t cfg = {.polynomial = HAL_CRC_POLY_CRC32,
                          .init_value = 0xFFFFFFFFu};
  hal_crc_init(&cfg);
}

void test_crc_sw_empty_returns_init(void) {
  init_default();
  TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFu, hal_crc_compute(NULL, 0));
}

void test_crc_sw_single_byte(void) {
  init_default();
  const uint8_t data = 0x00;
  /* CRC-32/MPEG-2 of {0x00} with init 0xFFFFFFFF.
   *   start = 0xFFFFFFFF
   *   index = (start >> 24) ^ 0x00 = 0xFF
   *   crc   = (start << 8) ^ table[0xFF]
   *         = 0xFFFFFF00 ^ 0xB1F740B4 = 0x4E08BFB4 */
  TEST_ASSERT_EQUAL_UINT32(0x4E08BFB4u, hal_crc_compute(&data, 1));
}

void test_crc_sw_mpeg2_reference_vector(void) {
  init_default();
  /* "123456789" — the canonical Rocksoft / pycrc CRC-32/MPEG-2 vector. */
  const uint8_t s[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
  TEST_ASSERT_EQUAL_UINT32(0x0376E6E7u, hal_crc_compute(s, sizeof(s)));
}

void test_crc_sw_accumulate_matches_compute(void) {
  init_default();
  const uint8_t s[] = "navhal-crc";
  const uint32_t n = (uint32_t)(sizeof(s) - 1);
  uint32_t single = hal_crc_compute(s, n);

  hal_crc_reset();
  uint32_t a = hal_crc_accumulate(s, 4);
  (void)a;
  uint32_t streamed = hal_crc_accumulate(s + 4, n - 4);
  TEST_ASSERT_EQUAL_UINT32(single, streamed);
}

void test_crc_sw_reset_restores_init(void) {
  init_default();
  const uint8_t s[] = {0xDE, 0xAD, 0xBE, 0xEF};
  hal_crc_accumulate(s, sizeof(s));
  hal_crc_reset();
  /* After reset, accumulating nothing yields the init value. */
  TEST_ASSERT_EQUAL_UINT32(0xFFFFFFFFu, hal_crc_accumulate(NULL, 0));
}

static const navtest_case_t crc_sw_cases[] = {
    NAVTEST_CASE(test_crc_sw_init_rejects_null),
    NAVTEST_CASE(test_crc_sw_empty_returns_init),
    NAVTEST_CASE(test_crc_sw_single_byte),
    NAVTEST_CASE(test_crc_sw_mpeg2_reference_vector),
    NAVTEST_CASE(test_crc_sw_accumulate_matches_compute),
    NAVTEST_CASE(test_crc_sw_reset_restores_init),
};

const navtest_suite_t test_crc_sw_suite = {
    .name = "CRC SOFTWARE (host)",
    .cases = crc_sw_cases,
    .count = sizeof(crc_sw_cases) / sizeof(crc_sw_cases[0]),
    .between = NULL,
};
