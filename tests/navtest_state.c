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

#if defined(__AVR__)
/* PROGMEM-aware variant. Reads bytes from program flash one at a time
 * and feeds them to the UART — avoids the .data copy that plain
 * literals incur on AVR (Harvard architecture). On non-AVR targets
 * navtest_write_P is `#define`d to navtest_write in navtest.h, so this
 * symbol only exists on AVR. */
#include <avr/pgmspace.h>
void navtest_write_P(const char *p) {
  for (;;) {
    char c = (char)pgm_read_byte(p++);
    if (c == '\0') {
      break;
    }
    hal_uart_write_char(NAVTEST_UART, (uint8_t)c);
  }
}
#endif
#endif /* !NAVTEST_HOST */

/* Global state */
_NavTestState _navtest = {0, 0, 0};

/* Default setUp / tearDown — weak so any test file can override */
__attribute__((weak)) void setUp(void) {}
__attribute__((weak)) void tearDown(void) {}

/* Walk a suite's case table, printing per-suite banners + summary.
 * Identical per-case output to RUN_TEST.
 *
 * Case names live in PROGMEM on AVR (NAVTEST_CASE_DECL declares each
 * one as a static const char[] __progmem__) and in .rodata everywhere
 * else. navtest_write_P aliases navtest_write on non-AVR, so reading
 * c->name through it is portable.
 *
 * Suite names are still RAM string literals — short, few per build,
 * not worth a parallel _DECL macro. */
int navtest_run_suite(const navtest_suite_t *suite) {
  navtest_write_P(_NT_PSTR("\n=========== "));
  navtest_write(suite->name);
  navtest_write_P(_NT_PSTR(" TEST START ===========\n"));

  _navtest.tests = 0;
  _navtest.failures = 0;
  _navtest.passes = 0;

  for (size_t i = 0; i < suite->count; i++) {
    const navtest_case_t *c = &suite->cases[i];
    _navtest.tests++;
    uint32_t prev = _navtest.failures;
    setUp();
    navtest_write_P(_NT_PSTR(_NT_CYAN "  >> " _NT_BOLD));
    navtest_write_P(c->name);
    navtest_write_P(_NT_PSTR(_NT_RST "\r\n"));
    c->fn();
    if (_navtest.failures == prev) {
      _navtest.passes++;
      navtest_write_P(_NT_PSTR(_NT_BOLD _NT_GREEN "  PASS" _NT_RST "\r\n"));
    }
    tearDown();
    if (suite->between)
      suite->between();
  }

  _navtest_end_impl();
  navtest_write_P(_NT_PSTR("=========== "));
  navtest_write(suite->name);
  navtest_write_P(_NT_PSTR(" TEST END ===========\n"));

  return (int)_navtest.failures;
}
