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

#include "test_adc.h"
#include "family/adc_reg.h"
#include "family/rcc_reg.h"
#include "navhal_port_adc.h"
#include "navtest/navtest.h"
#include "navtest/navtest_pil.h"
#include <stdint.h>

void test_adc_init_returns_ok(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_adc_init(HAL_ADC_1, NULL));
}

void test_adc_init_rejects_bad_unit(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_adc_init((hal_adc_t)99, NULL));
}

void test_adc_clock_enabled_after_init(void) {
  hal_adc_init(HAL_ADC_1, NULL);
  TEST_ASSERT_BITS_HIGH(RCC_APB2ENR_ADC1EN, RCC->APB2ENR);
}

void test_adc_read_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_adc_read(HAL_ADC_1, 0, NULL));
}

/* Device-free: a floating input still converts to *some* code — this proves the
 * ADC actually runs a conversion (EOC fires) and the result is 12-bit-wide. */
void test_adc_read_completes_and_in_range(void) {
  NAVTEST_SKIP_ON_PIL(); /* the emulator may not model an ADC conversion */
  hal_adc_init(HAL_ADC_1, NULL);
  uint16_t v = 0xFFFFu;
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_adc_read(HAL_ADC_1, 0, &v));
  TEST_ASSERT_TRUE(v <= 0x0FFFu); /* 12-bit right-aligned */
}

NAVTEST_CASE_DECL(test_adc_init_returns_ok);
NAVTEST_CASE_DECL(test_adc_init_rejects_bad_unit);
NAVTEST_CASE_DECL(test_adc_clock_enabled_after_init);
NAVTEST_CASE_DECL(test_adc_read_rejects_null);
NAVTEST_CASE_DECL(test_adc_read_completes_and_in_range);

static const navtest_case_t adc_cases[] = {
    NAVTEST_CASE(test_adc_init_returns_ok),
    NAVTEST_CASE(test_adc_init_rejects_bad_unit),
    NAVTEST_CASE(test_adc_clock_enabled_after_init),
    NAVTEST_CASE(test_adc_read_rejects_null),
    NAVTEST_CASE(test_adc_read_completes_and_in_range),
};

const navtest_suite_t test_adc_suite = {
    .name = "ADC",
    .cases = adc_cases,
    .count = sizeof(adc_cases) / sizeof(adc_cases[0]),
    .between = NULL,
};
