/**
 * @file navtest_state.c
 * @brief NavTest global state — compiled exactly once.
 *
 * Defines the framework's global counters and the jmp_buf used by RUN_TEST.
 * Also provides weak no-op implementations of setUp/tearDown that test files
 * can override with their own definitions.
 */

#include "navtest/navtest.h"

/* Global state */
_NavTestState _navtest = {0, 0, 0};

/* Default setUp / tearDown — weak so any test file can override */
__attribute__((weak)) void setUp(void) {}
__attribute__((weak)) void tearDown(void) {}
