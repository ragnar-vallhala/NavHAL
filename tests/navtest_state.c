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
 * @file navtest_state.c
 * @brief NavTest global state — compiled exactly once.
 *
 * Defines the framework's global counters and the jmp_buf used by RUN_TEST.
 * Also provides weak no-op implementations of setUp/tearDown that test files
 * can override with their own definitions.
 */

#include "navtest/navtest.h"

#ifndef NAVTEST_HOST
/* Target backend: route navtest output to the per-target console UART
 * (NAVTEST_UART, defined in tests/navtest_target.h per arch). The host
 * backend lives in tests/host/host_backend.c and routes to stdout. */
#include "navhal_port_uart.h"
#include "navtest_target.h"
void navtest_write(const char *s) { hal_uart_print(NAVTEST_UART, (char *)s); }
#endif

/* Global state */
_NavTestState _navtest = {0, 0, 0};

/* Default setUp / tearDown — weak so any test file can override */
__attribute__((weak)) void setUp(void) {}
__attribute__((weak)) void tearDown(void) {}

/* Walk a suite's case table, printing per-suite banners + summary.
 * Identical per-case output to RUN_TEST. */
int navtest_run_suite(const navtest_suite_t *suite) {
  navtest_write("\n=========== ");
  navtest_write(suite->name);
  navtest_write(" TEST START ===========\n");

  _navtest.tests = 0;
  _navtest.failures = 0;
  _navtest.passes = 0;

  for (size_t i = 0; i < suite->count; i++) {
    const navtest_case_t *c = &suite->cases[i];
    _navtest.tests++;
    uint32_t prev = _navtest.failures;
    setUp();
    navtest_write(_NT_CYAN "  >> " _NT_BOLD);
    navtest_write(c->name);
    navtest_write(_NT_RST "\r\n");
    c->fn();
    if (_navtest.failures == prev) {
      _navtest.passes++;
      navtest_write(_NT_BOLD _NT_GREEN "  PASS" _NT_RST "\r\n");
    }
    tearDown();
    if (suite->between)
      suite->between();
  }

  _navtest_end_impl();
  navtest_write("=========== ");
  navtest_write(suite->name);
  navtest_write(" TEST END ===========\n");

  return (int)_navtest.failures;
}
