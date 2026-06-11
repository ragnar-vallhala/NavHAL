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
 * Runs on AVR too: TEST_ASSERT_* macros and case names both land in
 * PROGMEM on AVR (assertion strings via `_NT_PSTR()`, case names via
 * the `NAVTEST_CASE_DECL` predeclaration trick), keeping the suite
 * well under the ATmega328P's 2 KB SRAM ceiling.
 */
#ifndef TEST_CONFORMANCE_H
#define TEST_CONFORMANCE_H

#include "navtest/navtest.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Status type contract — error codes are non-zero, HAL_OK is zero,
 * codes are pairwise distinct, and each fits in 8 bits (RPC ABI). */
void test_conformance_status_ok_is_zero(void);
void test_conformance_status_errors_distinct(void);
void test_conformance_status_fits_uint8(void);

/* HAL_OK_OR_RETURN macro contract — passes the status through on OK,
 * short-circuits on non-OK, and evaluates its argument exactly once. */
void test_conformance_hal_ok_or_return_passes_through(void);
void test_conformance_hal_ok_or_return_short_circuits(void);

/* Null-pointer contract — every public init function that takes a
 * pointer must reject NULL with HAL_ERR_INVALID_ARG, not crash; and
 * calling the NULL path twice in a row must return the same status
 * both times (no state corruption on the error branch). */
void test_conformance_uart_init_rejects_null(void);
void test_conformance_clock_init_rejects_null(void);
void test_conformance_dma_init_rejects_null(void);
void test_conformance_i2c_init_rejects_null(void);
void test_conformance_spi_init_rejects_null(void);
void test_conformance_pwm_init_rejects_null(void);
void test_conformance_sdio_init_rejects_null(void);
void test_conformance_gpio_init_rejects_null(void);
void test_conformance_null_init_is_idempotent(void);

/* Capability-flag contract — NAVHAL_HAS_X is always 0 or 1, never
 * unset (#ifdef NAVHAL_HAS_X yields a value; the macro is a contract,
 * not a feature switch). */
void test_conformance_cap_macros_are_defined(void);

extern const navtest_suite_t test_conformance_suite;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* TEST_CONFORMANCE_H */
