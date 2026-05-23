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
/* Target backend: route navtest output to UART2. The host backend lives in
 * tests/host/host_backend.c and routes to stdout. */
#include "navhal_port_uart.h"
void navtest_write(const char *s) { uart2_write(s); }
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
