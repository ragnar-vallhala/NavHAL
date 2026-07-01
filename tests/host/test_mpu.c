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
 * @file tests/host/test_mpu.c
 * @brief Host tests for the ARMv7-M MPU driver (src/arch/armv7e-m/mpu/mpu.c).
 *
 * @details
 * Runs the *real* driver against the simulated System Control Space that
 * host_mmio maps at 0xE000E000 (see host_mmio.c). The backing store is flat
 * memory — it does not model the hardware's region banking — so the register
 * assertions verify the driver's field packing and control flow (which is
 * where port bugs live), while the RBAR/RASR bit-patterns are checked against
 * hand-computed known-good values. Region presence is seeded per-test by
 * writing MPU_TYPE.DREGION directly, so both the "MPU present" and "no MPU"
 * contracts are exercised on the host.
 */

#include "common/hal_mpu.h"
#include "test_mpu.h"
#include <stdint.h>

/* System Control Space MPU block (0xE000ED90), matching mpu.c. */
#define MPU_TYPE (*(volatile uint32_t *)0xE000ED90UL)
#define MPU_CTRL (*(volatile uint32_t *)0xE000ED94UL)
#define MPU_RNR  (*(volatile uint32_t *)0xE000ED98UL)
#define MPU_RBAR (*(volatile uint32_t *)0xE000ED9CUL)
#define MPU_RASR (*(volatile uint32_t *)0xE000EDA0UL)

/* Seed the simulated MPU with @p regions banks and a cleared control state. */
static void seed_regions(uint32_t regions) {
  MPU_TYPE = (regions & 0xFFu) << 8; /* DREGION field is bits [15:8] */
  MPU_CTRL = 0u;
  MPU_RNR = 0u;
  MPU_RBAR = 0u;
  MPU_RASR = 0u;
}

/* Known-good descriptor A: 32 KB normal write-back, full RW, non-exec,
 * shareable, at 0x20000000, region 3.
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

/* Known-good descriptor B: 256 B device, privileged-RW, non-exec, region 0.
 *   RASR = EN | (7<<1) | B | (AP_PRIV_RW=1<<24) | XN = 0x1101000F
 *   RBAR = 0x40020000 | VALID | 0                    = 0x40020010          */
static const hal_mpu_region_t region_b = {
    .base = 0x40020000u,
    .size = HAL_MPU_SIZE_256B,
    .ap = HAL_MPU_AP_PRIV_RW,
    .mem = HAL_MPU_MEM_DEVICE,
    .executable = false,
    .shareable = false,
    .srd_mask = 0u,
};
#define REGION_B_IDX 0u
#define REGION_B_RASR 0x1101000Fu
#define REGION_B_RBAR 0x40020010u

/* -------------------------------------------------------------------------- */

void test_mpu_absent_reports_not_supported(void) {
  seed_regions(0); /* DREGION == 0 => no MPU */
  hal_mpu_encoded_t enc;

  TEST_ASSERT_FALSE(hal_mpu_present());
  TEST_ASSERT_EQUAL_UINT32(0u, hal_mpu_num_regions());
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_NOT_SUPPORTED,
                           (uint32_t)hal_mpu_enable(true));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_NOT_SUPPORTED,
                           (uint32_t)hal_mpu_disable());
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_NOT_SUPPORTED,
                           (uint32_t)hal_mpu_configure_region(0, &region_a));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_NOT_SUPPORTED,
                           (uint32_t)hal_mpu_disable_region(0));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_NOT_SUPPORTED,
                           (uint32_t)hal_mpu_encode(0, &region_a, &enc));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_NOT_SUPPORTED,
                           (uint32_t)hal_mpu_apply(&enc, 1));
}

void test_mpu_present_reports_region_count(void) {
  seed_regions(16); /* Cortex-M7 */
  TEST_ASSERT_TRUE(hal_mpu_present());
  TEST_ASSERT_EQUAL_UINT32(16u, hal_mpu_num_regions());

  seed_regions(8); /* Cortex-M4 */
  TEST_ASSERT_TRUE(hal_mpu_present());
  TEST_ASSERT_EQUAL_UINT32(8u, hal_mpu_num_regions());
}

void test_mpu_encode_normal_wb_vector(void) {
  seed_regions(16);
  hal_mpu_encoded_t enc = {0};
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_mpu_encode(REGION_A_IDX, &region_a,
                                                    &enc));
  TEST_ASSERT_EQUAL_UINT32(REGION_A_RASR, enc.rasr);
  TEST_ASSERT_EQUAL_UINT32(REGION_A_RBAR, enc.rbar);
}

void test_mpu_encode_device_vector(void) {
  seed_regions(16);
  hal_mpu_encoded_t enc = {0};
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_mpu_encode(REGION_B_IDX, &region_b,
                                                    &enc));
  TEST_ASSERT_EQUAL_UINT32(REGION_B_RASR, enc.rasr);
  TEST_ASSERT_EQUAL_UINT32(REGION_B_RBAR, enc.rbar);
}

void test_mpu_encode_rejects_bad_args(void) {
  seed_regions(8);
  hal_mpu_encoded_t enc = {0};

  /* NULL descriptor and NULL output. */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_mpu_encode(0, NULL, &enc));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_mpu_encode(0, &region_a, NULL));

  /* Index at/above the region count (8 banks => valid 0..7). */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_mpu_encode(8, &region_a, &enc));

  /* Base not aligned to the 32 KB region size. */
  hal_mpu_region_t misaligned = region_a;
  misaligned.base = 0x20000004u;
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_mpu_encode(0, &misaligned, &enc));

  /* Subregion-disable mask on a sub-256 B region is illegal. */
  hal_mpu_region_t small = region_a;
  small.size = HAL_MPU_SIZE_128B;
  small.base = 0x20000000u; /* aligned to 128 B */
  small.srd_mask = 0x01u;
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_mpu_encode(0, &small, &enc));
}

void test_mpu_enable_disable_ctrl(void) {
  seed_regions(16);

  /* Background-privileged enable: ENABLE(bit0) | PRIVDEFENA(bit2) = 0x5. */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_mpu_enable(true));
  TEST_ASSERT_BITS_HIGH(0x5u, MPU_CTRL);
  TEST_ASSERT_BITS_LOW(0x2u, MPU_CTRL); /* HFNMIENA stays off */

  /* Without the background region: ENABLE only, PRIVDEFENA clear. */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_mpu_enable(false));
  TEST_ASSERT_BITS_HIGH(0x1u, MPU_CTRL);
  TEST_ASSERT_BITS_LOW(0x4u, MPU_CTRL);

  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_mpu_disable());
  TEST_ASSERT_EQUAL_UINT32(0u, MPU_CTRL);
}

void test_mpu_configure_region_writes_regs(void) {
  seed_regions(16);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_mpu_configure_region(REGION_A_IDX,
                                                              &region_a));
  TEST_ASSERT_EQUAL_UINT32(REGION_A_IDX, MPU_RNR);
  TEST_ASSERT_EQUAL_UINT32(REGION_A_RBAR, MPU_RBAR);
  TEST_ASSERT_EQUAL_UINT32(REGION_A_RASR, MPU_RASR);
  TEST_ASSERT_BITS_HIGH(0x1u, MPU_RASR); /* region left enabled */
}

void test_mpu_disable_region_clears_enable(void) {
  seed_regions(16);
  hal_mpu_configure_region(REGION_A_IDX, &region_a);
  TEST_ASSERT_BITS_HIGH(0x1u, MPU_RASR);

  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_mpu_disable_region(REGION_A_IDX));
  TEST_ASSERT_EQUAL_UINT32(REGION_A_IDX, MPU_RNR);
  TEST_ASSERT_BITS_LOW(0x1u, MPU_RASR); /* enable bit cleared */
}

void test_mpu_apply_writes_each_pair(void) {
  seed_regions(16);
  hal_mpu_encoded_t set[2] = {0};
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_mpu_encode(REGION_A_IDX, &region_a,
                                                    &set[0]));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_mpu_encode(REGION_B_IDX, &region_b,
                                                    &set[1]));

  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_mpu_apply(set, 2));
  /* Flat backing store: the last pair written stays in RBAR/RASR. */
  TEST_ASSERT_EQUAL_UINT32(REGION_B_RBAR, MPU_RBAR);
  TEST_ASSERT_EQUAL_UINT32(REGION_B_RASR, MPU_RASR);

  /* count above the region count, and a NULL set, are rejected. */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_mpu_apply(set, 17));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_mpu_apply(NULL, 1));
}

/* PROGMEM slot for each case name on AVR; no-op on the host. */
NAVTEST_CASE_DECL(test_mpu_absent_reports_not_supported);
NAVTEST_CASE_DECL(test_mpu_present_reports_region_count);
NAVTEST_CASE_DECL(test_mpu_encode_normal_wb_vector);
NAVTEST_CASE_DECL(test_mpu_encode_device_vector);
NAVTEST_CASE_DECL(test_mpu_encode_rejects_bad_args);
NAVTEST_CASE_DECL(test_mpu_enable_disable_ctrl);
NAVTEST_CASE_DECL(test_mpu_configure_region_writes_regs);
NAVTEST_CASE_DECL(test_mpu_disable_region_clears_enable);
NAVTEST_CASE_DECL(test_mpu_apply_writes_each_pair);

static const navtest_case_t mpu_cases[] = {
    NAVTEST_CASE(test_mpu_absent_reports_not_supported),
    NAVTEST_CASE(test_mpu_present_reports_region_count),
    NAVTEST_CASE(test_mpu_encode_normal_wb_vector),
    NAVTEST_CASE(test_mpu_encode_device_vector),
    NAVTEST_CASE(test_mpu_encode_rejects_bad_args),
    NAVTEST_CASE(test_mpu_enable_disable_ctrl),
    NAVTEST_CASE(test_mpu_configure_region_writes_regs),
    NAVTEST_CASE(test_mpu_disable_region_clears_enable),
    NAVTEST_CASE(test_mpu_apply_writes_each_pair),
};

const navtest_suite_t test_mpu_suite = {
    .name = "MPU DRIVER (host, simulated SCS)",
    .cases = mpu_cases,
    .count = sizeof(mpu_cases) / sizeof(mpu_cases[0]),
    .between = NULL,
};
