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
 * @file test_conformance.h
 * @brief HAL contract conformance suite — runs on every port (almost).
 *
 * Mechanical assertions of properties the HAL contract documents
 * in `include/common/` docstrings and `docs/api_standardization.md`.
 * A port that compiles + links + passes this suite implements the
 * v1 contract correctly; one that fails is provably non-conformant.
 *
 * Each test is **black-box** — it calls only the public hal_* API
 * and asserts behaviour from outside. No register access, no
 * vendor-specific includes. The whole file lives under
 * tests/portable/ for exactly that reason.
 *
 * Runs on AVR too as of the navtest PROGMEM-string support (M7
 * follow-up): TEST_ASSERT_* macros now route __FILE__ + assertion
 * messages through `_NT_PSTR()` so those strings live in flash on
 * AVR rather than .data, freeing ~440 bytes of SRAM. Case names
 * (`#fn` stringification inside file-scope NAVTEST_CASE) stay in
 * RAM because GCC statement-expressions aren't allowed in static
 * initializers — see the NAVTEST_CASE comment in navtest.h.
 */
#ifndef TEST_CONFORMANCE_H
#define TEST_CONFORMANCE_H

#include "navtest/navtest.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Status type contract — error codes are non-zero, HAL_OK is zero,
 * codes are distinct from each other. */
void test_conformance_status_ok_is_zero(void);
void test_conformance_status_errors_distinct(void);

/* Null-pointer contract — every public init function that takes a
 * pointer must reject NULL with HAL_ERR_INVALID_ARG, not crash. */
void test_conformance_uart_init_rejects_null(void);
void test_conformance_clock_init_rejects_null(void);
void test_conformance_dma_init_rejects_null(void);
void test_conformance_i2c_init_rejects_null(void);
void test_conformance_spi_init_rejects_null(void);
void test_conformance_pwm_init_rejects_null(void);
void test_conformance_sdio_init_rejects_null(void);
void test_conformance_gpio_init_rejects_null(void);

/* Capability-flag contract — NAVHAL_HAS_X is always 0 or 1, never
 * unset (#ifdef NAVHAL_HAS_X yields a value; the macro is a contract,
 * not a feature switch). */
void test_conformance_cap_macros_are_defined(void);

extern const navtest_suite_t test_conformance_suite;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TEST_CONFORMANCE_H */
