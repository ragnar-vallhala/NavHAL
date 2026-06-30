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
 * @file test_clock_driver.c
 * @brief Deep host (SIL) tests for clock_f7.c clock-getter math against
 *        simulated RCC. Pre-seeds CFGR (source + prescalers) and PLLCFGR and
 *        asserts get_sysclk / ahb / apb1 / apb2 decode correctly.
 */

#include "host_mmio.h"
#include "navhal_port_clock.h"
#include "family/rcc_reg.h"
#include "navtest/navtest.h"
#include <stdint.h>

static void set_sws(uint32_t sws) {
  RCC->CFGR = (RCC->CFGR & ~(0x3u << RCC_CFGR_SWS_BIT)) |
              ((sws & 0x3u) << RCC_CFGR_SWS_BIT);
}

void test_host_clock_sysclk_hsi(void) {
  host_mmio_reset();
  set_sws(0); /* HSI */
  TEST_ASSERT_EQUAL_UINT32(16000000u, hal_clock_get_sysclk());
}

void test_host_clock_sysclk_hse(void) {
  host_mmio_reset();
  set_sws(1); /* HSE = 8 MHz */
  TEST_ASSERT_EQUAL_UINT32(8000000u, hal_clock_get_sysclk());
}

void test_host_clock_sysclk_pll_from_hsi(void) {
  host_mmio_reset();
  /* HSI/8 * 216 / 2 = 216 MHz. SRC=0 (HSI). */
  RCC->PLLCFGR = RCC_PLLCFGR_PLLM(8) | RCC_PLLCFGR_PLLN(216) |
                 RCC_PLLCFGR_PLLP(2);
  set_sws(2); /* PLL */
  TEST_ASSERT_EQUAL_UINT32(216000000u, hal_clock_get_sysclk());
}

void test_host_clock_sysclk_pll_from_hse(void) {
  host_mmio_reset();
  /* HSE(8)/4 * 168 / 2 = 168 MHz. SRC=1 (HSE). */
  RCC->PLLCFGR = RCC_PLLCFGR_SRC | RCC_PLLCFGR_PLLM(4) | RCC_PLLCFGR_PLLN(168) |
                 RCC_PLLCFGR_PLLP(2);
  set_sws(2);
  TEST_ASSERT_EQUAL_UINT32(168000000u, hal_clock_get_sysclk());
}

void test_host_clock_ahb_prescaler(void) {
  host_mmio_reset();
  set_sws(0); /* 16 MHz */
  RCC->CFGR |= (RCC_CFGR_HPRE_DIV4 << RCC_CFGR_HPRE_BIT);
  TEST_ASSERT_EQUAL_UINT32(4000000u, hal_clock_get_ahbclk());
}

void test_host_clock_apb1_prescaler(void) {
  host_mmio_reset();
  set_sws(0);
  RCC->CFGR |= (RCC_CFGR_PPRE_DIV4 << RCC_CFGR_PPRE1_BIT);
  TEST_ASSERT_EQUAL_UINT32(4000000u, hal_clock_get_apb1clk());
}

void test_host_clock_apb2_prescaler(void) {
  host_mmio_reset();
  set_sws(0);
  RCC->CFGR |= (RCC_CFGR_PPRE_DIV2 << RCC_CFGR_PPRE2_BIT);
  TEST_ASSERT_EQUAL_UINT32(8000000u, hal_clock_get_apb2clk());
}

void test_host_clock_init_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_clock_init(NULL, NULL));
  hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_PLL};
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_clock_init(&cfg, NULL));
}

NAVTEST_CASE_DECL(test_host_clock_sysclk_hsi);
NAVTEST_CASE_DECL(test_host_clock_sysclk_hse);
NAVTEST_CASE_DECL(test_host_clock_sysclk_pll_from_hsi);
NAVTEST_CASE_DECL(test_host_clock_sysclk_pll_from_hse);
NAVTEST_CASE_DECL(test_host_clock_ahb_prescaler);
NAVTEST_CASE_DECL(test_host_clock_apb1_prescaler);
NAVTEST_CASE_DECL(test_host_clock_apb2_prescaler);
NAVTEST_CASE_DECL(test_host_clock_init_rejects_null);

static const navtest_case_t clock_driver_cases[] = {
    NAVTEST_CASE(test_host_clock_sysclk_hsi),
    NAVTEST_CASE(test_host_clock_sysclk_hse),
    NAVTEST_CASE(test_host_clock_sysclk_pll_from_hsi),
    NAVTEST_CASE(test_host_clock_sysclk_pll_from_hse),
    NAVTEST_CASE(test_host_clock_ahb_prescaler),
    NAVTEST_CASE(test_host_clock_apb1_prescaler),
    NAVTEST_CASE(test_host_clock_apb2_prescaler),
    NAVTEST_CASE(test_host_clock_init_rejects_null),
};

const navtest_suite_t test_clock_driver_suite = {
    .name = "CLOCK DRIVER (host)",
    .cases = clock_driver_cases,
    .count = sizeof(clock_driver_cases) / sizeof(clock_driver_cases[0]),
    .between = NULL,
};
