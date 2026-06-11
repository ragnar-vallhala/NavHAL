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
 * @file tests/host/main.c
 * @brief Host-runnable test entry point.
 *
 * Walks the host-only suite array and returns the number of failures —
 * suitable for `ctest` / CI exit-code checks.
 */

#include "navtest/navtest.h"
#include "test_conversion.h"
#include "test_crc_sw.h"
#include "test_gpio_encoding.h"
#include "test_hal_status.h"

#include <stdio.h>

static const navtest_suite_t *const host_suites[] = {
    &test_hal_status_suite,
    &test_conversion_suite,
    &test_crc_sw_suite,
    &test_gpio_encoding_suite,
};

int main(void) {
  navtest_write("\r\n"
                "|========================================|\r\n"
                "|        NAVHAL host-test suite          |\r\n"
                "|========================================|\r\n");

  int failed = 0;
  uint32_t total = 0;
  const size_t n = sizeof(host_suites) / sizeof(host_suites[0]);
  for (size_t i = 0; i < n; i++) {
    failed += navtest_run_suite(host_suites[i]);
    total += host_suites[i]->count;
  }

  navtest_write("\n=========== FINAL RESULTS ===========\n");
  navtest_write("Total tests run: ");
  _navtest_print_uint32(total);
  navtest_write("\nTotal failures:  ");
  _navtest_print_uint32((uint32_t)failed);
  navtest_write("\n");
  return failed;
}
