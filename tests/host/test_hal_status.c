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
 * @file tests/host/test_hal_status.c
 * @brief Host-runnable tests for the hal_status_t contract.
 *
 * The contract is part of the standardized API (see hal_status.h):
 *   - HAL_OK is guaranteed == 0.
 *   - HAL_ERR is guaranteed == 1.
 *   - The remaining codes are distinct positive values.
 *   - HAL_OK_OR_RETURN short-circuits the enclosing function on non-OK.
 *
 * Application code depends on these invariants — verify them on the host.
 */

#include "common/hal_status.h"
#include "test_hal_status.h"

void test_hal_status_ok_is_zero(void) {
  TEST_ASSERT_EQUAL_UINT32(0, (uint32_t)HAL_OK);
}

void test_hal_status_err_is_one(void) {
  TEST_ASSERT_EQUAL_UINT32(1, (uint32_t)HAL_ERR);
}

void test_hal_status_distinct_codes(void) {
  /* No two codes share a value. */
  hal_status_t codes[] = {
      HAL_OK,        HAL_ERR,        HAL_ERR_INVALID_ARG,
      HAL_ERR_TIMEOUT, HAL_ERR_BUSY,  HAL_ERR_NOT_INITIALIZED,
      HAL_ERR_NOT_SUPPORTED, HAL_ERR_IO, HAL_ERR_NO_MEM,
  };
  const size_t n = sizeof(codes) / sizeof(codes[0]);
  for (size_t i = 0; i < n; i++) {
    for (size_t j = i + 1; j < n; j++) {
      TEST_ASSERT_TRUE(codes[i] != codes[j]);
    }
  }
}

/* Helpers that exercise HAL_OK_OR_RETURN's macro behavior. */
static hal_status_t produce_ok(void) { return HAL_OK; }
static hal_status_t produce_timeout(void) { return HAL_ERR_TIMEOUT; }

static hal_status_t chain_ok_then_ok(int *side_effect) {
  HAL_OK_OR_RETURN(produce_ok());
  *side_effect = 1; /* must run */
  HAL_OK_OR_RETURN(produce_ok());
  *side_effect = 2; /* must run */
  return HAL_OK;
}

static hal_status_t chain_ok_then_err(int *side_effect) {
  HAL_OK_OR_RETURN(produce_ok());
  *side_effect = 1; /* must run */
  HAL_OK_OR_RETURN(produce_timeout());
  *side_effect = 999; /* MUST NOT run */
  return HAL_OK;
}

void test_hal_ok_or_return_passes_through(void) {
  int s = 0;
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)chain_ok_then_ok(&s));
  TEST_ASSERT_EQUAL_UINT32(2, (uint32_t)s);
}

void test_hal_ok_or_return_short_circuits(void) {
  int s = 0;
  hal_status_t got = chain_ok_then_err(&s);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_TIMEOUT, (uint32_t)got);
  TEST_ASSERT_EQUAL_UINT32(1, (uint32_t)s); /* stopped after the OK call */
}
/* PROGMEM slot for each case name on AVR; no-op elsewhere. */
NAVTEST_CASE_DECL(test_hal_status_ok_is_zero);
NAVTEST_CASE_DECL(test_hal_status_err_is_one);
NAVTEST_CASE_DECL(test_hal_status_distinct_codes);
NAVTEST_CASE_DECL(test_hal_ok_or_return_passes_through);
NAVTEST_CASE_DECL(test_hal_ok_or_return_short_circuits);


static const navtest_case_t hal_status_cases[] = {
    NAVTEST_CASE(test_hal_status_ok_is_zero),
    NAVTEST_CASE(test_hal_status_err_is_one),
    NAVTEST_CASE(test_hal_status_distinct_codes),
    NAVTEST_CASE(test_hal_ok_or_return_passes_through),
    NAVTEST_CASE(test_hal_ok_or_return_short_circuits),
};

const navtest_suite_t test_hal_status_suite = {
    .name = "HAL_STATUS (host)",
    .cases = hal_status_cases,
    .count = sizeof(hal_status_cases) / sizeof(hal_status_cases[0]),
    .between = NULL,
};
