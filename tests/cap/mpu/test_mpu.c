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
 * @file tests/cap/mpu/test_mpu.c
 * @brief On-target / PIL capability tests for the ARMv7-M MPU driver.
 *
 * @details
 * Runs on Cortex-M4 / Cortex-M7 (real board or Renode PIL) whenever
 * NAVHAL_CONFIG_DRV_MPU is enabled. Unlike the host suite — which seeds
 * MPU_TYPE against a simulated SCS — this suite reads the *real* MPU_TYPE, so
 * each case is written to hold whether or not the core (or the emulator model)
 * actually implements an MPU: if hal_mpu_present() the presence contract is
 * asserted (region count, known-good encoding, configure/disable round-trip);
 * if not, every entry point must report HAL_ERR_NOT_SUPPORTED. The MPU is never
 * globally enabled here, so a region write cannot fault the test harness.
 */

#include "test_mpu.h"
#include "common/hal_features.h"
#include "navtest/navtest.h"
#include <stdint.h>

#if NAVHAL_CONFIG_DRV_MPU
#include "common/hal_mpu.h"

/* 32 KB normal write-back, full RW, non-exec, shareable, at 0x20000000 (SRAM on
 * both the F401 and F767), region 3 — valid on an 8- or 16-region core.
 *   RASR = EN | (14<<1) | C | B | S | (AP_RW=3<<24) | XN = 0x1307001D
 *   RBAR = 0x20000000 | VALID | 3                        = 0x20000013     */
static const hal_mpu_region_t region_a = {
    .base = 0x20000000u,
    .size = HAL_MPU_SIZE_32KB,
    .ap = HAL_MPU_AP_RW,
    .mem = HAL_MPU_MEM_NORMAL_WB,
    .executable = false,
    .shareable = true,
    .srd_mask = 0u,
};
#define REGION_A_IDX 3u
#define REGION_A_RASR 0x1307001Du
#define REGION_A_RBAR 0x20000013u

void test_mpu_present_matches_num_regions(void) {
  if (hal_mpu_present()) {
    uint32_t n = hal_mpu_num_regions();
    /* ARMv7-M cores ship 8 (M4) or 16 (M7) MPU regions. */
    TEST_ASSERT_TRUE(n == 8u || n == 16u);
  } else {
    TEST_ASSERT_EQUAL_UINT32(0u, hal_mpu_num_regions());
  }
}

void test_mpu_encode_vector_or_not_supported(void) {
  hal_mpu_encoded_t enc = {0};
  hal_status_t st = hal_mpu_encode(REGION_A_IDX, &region_a, &enc);
  if (hal_mpu_present()) {
    TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)st);
    TEST_ASSERT_EQUAL_UINT32(REGION_A_RASR, enc.rasr);
    TEST_ASSERT_EQUAL_UINT32(REGION_A_RBAR, enc.rbar);
  } else {
    TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_NOT_SUPPORTED, (uint32_t)st);
  }
}

void test_mpu_configure_roundtrip(void) {
  hal_status_t cfg = hal_mpu_configure_region(0, &region_a);
  if (hal_mpu_present()) {
    /* MPU left globally disabled, so programming region 0 over SRAM is inert. */
    TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)cfg);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                             (uint32_t)hal_mpu_disable_region(0));
  } else {
    TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_NOT_SUPPORTED, (uint32_t)cfg);
  }
}

void test_mpu_rejects_bad_args(void) {
  /* Absent parts fail the presence check first, so NULL args surface as
   * NOT_SUPPORTED there; on a real MPU the same calls are INVALID_ARG. */
  hal_status_t want =
      hal_mpu_present() ? HAL_ERR_INVALID_ARG : HAL_ERR_NOT_SUPPORTED;
  hal_mpu_encoded_t enc = {0};
  TEST_ASSERT_EQUAL_UINT32((uint32_t)want,
                           (uint32_t)hal_mpu_encode(0, NULL, &enc));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)want,
                           (uint32_t)hal_mpu_configure_region(0, NULL));
}

/* PROGMEM slot for each case name on AVR; no-op elsewhere (this suite never
 * links on AVR — MPU depends on Cortex-M — but keep the portable idiom). */
NAVTEST_CASE_DECL(test_mpu_present_matches_num_regions);
NAVTEST_CASE_DECL(test_mpu_encode_vector_or_not_supported);
NAVTEST_CASE_DECL(test_mpu_configure_roundtrip);
NAVTEST_CASE_DECL(test_mpu_rejects_bad_args);

static const navtest_case_t mpu_cases[] = {
    NAVTEST_CASE(test_mpu_present_matches_num_regions),
    NAVTEST_CASE(test_mpu_encode_vector_or_not_supported),
    NAVTEST_CASE(test_mpu_configure_roundtrip),
    NAVTEST_CASE(test_mpu_rejects_bad_args),
};

const navtest_suite_t test_mpu_suite = {
    .name = "MPU (cap)",
    .cases = mpu_cases,
    .count = sizeof(mpu_cases) / sizeof(mpu_cases[0]),
    .between = NULL,
};

#endif /* NAVHAL_CONFIG_DRV_MPU */
