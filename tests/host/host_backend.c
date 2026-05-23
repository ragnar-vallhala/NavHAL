/**
 * @file host_backend.c
 * @brief navtest output backend for the host build — writes to stdout.
 *
 * The on-target backend lives in tests/navtest_state.c and routes to UART2.
 * Selection is via the `NAVTEST_HOST` compile definition (set by
 * tests/host/CMakeLists.txt).
 */

#include "navtest/navtest.h"
#include <stdio.h>

void navtest_write(const char *s) { fputs(s, stdout); }
