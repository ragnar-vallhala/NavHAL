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
 * @file tests/cap/cache/test_cache.c
 * @brief On-target / PIL capability tests for the Cortex-M7 L1-cache driver.
 *
 * @details
 * Runs whenever NAVHAL_CONFIG_DRV_CACHE is enabled (Cortex-M7 only). The enable
 * call is always expected to succeed; the "is it actually enabled" bit-check is
 * skipped under PIL, since a Renode model may not reflect SCB_CCR.IC.
 */

#include "test_cache.h"
#include "common/hal_features.h"
#include "navtest/navtest.h"
#include "navtest/navtest_pil.h"

#if NAVHAL_CONFIG_DRV_CACHE
#include "common/hal_cache.h"

void test_icache_enable_returns_ok(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_icache_enable());
}

void test_icache_enabled_after_enable(void) {
  NAVTEST_SKIP_ON_PIL(); /* SCB_CCR.IC may not be modelled by the emulator */
  hal_icache_enable();
  TEST_ASSERT_TRUE(hal_icache_is_enabled());
}

void test_dcache_enable_returns_ok(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_dcache_enable());
}

void test_dcache_enabled_after_enable(void) {
  NAVTEST_SKIP_ON_PIL(); /* SCB_CCR.DC may not be modelled by the emulator */
  hal_dcache_enable();
  TEST_ASSERT_TRUE(hal_dcache_is_enabled());
}

/* Drive the by-MVA clean/invalidate loops over a real cache-line-aligned buffer.
 * A bad SCB maintenance-register address or a broken line-walk would fault here;
 * the data check confirms a clean+invalidate round-trip preserves the bytes. */
void test_dcache_maintenance_smoke(void) {
  static uint8_t buf[NAVHAL_CACHE_LINE * 2] NAVHAL_DMA_ALIGN;
  for (unsigned i = 0; i < sizeof buf; i++)
    buf[i] = (uint8_t)(i + 1U);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_dcache_clean(buf, sizeof buf));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_dcache_clean_invalidate(buf, sizeof buf));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_dcache_invalidate(buf, sizeof buf));
  bool intact = true;
  for (unsigned i = 0; i < sizeof buf; i++)
    intact = intact && (buf[i] == (uint8_t)(i + 1U));
  TEST_ASSERT_TRUE(intact);
}

/* Disable must write dirty lines back and return. (The stack corruption it
 * used to suffer -- see hal_dcache_disable -- shows in a vaios image on the
 * F767 but not in this suite, so this checks the write-back, not that.) */
void test_dcache_disable_writes_back_and_returns(void) {
  static uint32_t buf[NAVHAL_CACHE_LINE] NAVHAL_DMA_ALIGN;
  hal_dcache_enable();
  for (unsigned i = 0; i < NAVHAL_CACHE_LINE; i++)
    buf[i] = 0xC0DE0000u + i;
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_dcache_disable());
  bool off = !hal_dcache_is_enabled();
  bool intact = true; /* read from memory now: the cache is off */
  for (unsigned i = 0; i < NAVHAL_CACHE_LINE; i++)
    intact = intact && buf[i] == 0xC0DE0000u + i;
  hal_dcache_enable();
  TEST_ASSERT_TRUE(intact);
  NAVTEST_SKIP_ON_PIL(); /* SCB_CCR.DC may not be modelled by the emulator */
  TEST_ASSERT_TRUE(off);
}

/* PROGMEM slot for each case name on AVR; no-op elsewhere (never links on AVR —
 * the cache cap is Cortex-M7 only — but keep the portable idiom). */
NAVTEST_CASE_DECL(test_icache_enable_returns_ok);
NAVTEST_CASE_DECL(test_icache_enabled_after_enable);
NAVTEST_CASE_DECL(test_dcache_enable_returns_ok);
NAVTEST_CASE_DECL(test_dcache_enabled_after_enable);
NAVTEST_CASE_DECL(test_dcache_maintenance_smoke);
NAVTEST_CASE_DECL(test_dcache_disable_writes_back_and_returns);

static const navtest_case_t cache_cases[] = {
    NAVTEST_CASE(test_icache_enable_returns_ok),
    NAVTEST_CASE(test_icache_enabled_after_enable),
    NAVTEST_CASE(test_dcache_enable_returns_ok),
    NAVTEST_CASE(test_dcache_enabled_after_enable),
    NAVTEST_CASE(test_dcache_maintenance_smoke),
    NAVTEST_CASE(test_dcache_disable_writes_back_and_returns),
};

const navtest_suite_t test_cache_suite = {
    .name = "CACHE (cap)",
    .cases = cache_cases,
    .count = sizeof(cache_cases) / sizeof(cache_cases[0]),
    .between = NULL,
};

#endif /* NAVHAL_CONFIG_DRV_CACHE */
