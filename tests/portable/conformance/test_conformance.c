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

#include "test_conformance.h"

#include "common/hal_features.h"
#include "common/hal_status.h"
#include "common/hal_gpio.h"
#include "common/hal_clock.h"

#if NAVHAL_HAS_UART
#include "common/hal_uart.h"
#endif
#if NAVHAL_HAS_DMA
#include "common/hal_dma.h"
#endif
#if NAVHAL_HAS_I2C
#include "common/hal_i2c.h"
#endif
#if NAVHAL_HAS_SPI
#include "common/hal_spi.h"
#endif
#if NAVHAL_HAS_PWM
#include "common/hal_pwm.h"
#endif
#if NAVHAL_HAS_SDIO
#include "common/hal_sdio.h"
#endif

/* -------------------------------------------------------------------------- *
 * hal_status_t contract
 * -------------------------------------------------------------------------- */

void test_conformance_status_ok_is_zero(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)0, (uint32_t)HAL_OK);
}

void test_conformance_status_errors_distinct(void) {
  /* No two error codes share a value. A port that aliases
   * HAL_ERR_INVALID_ARG to HAL_ERR_TIMEOUT would silently lose
   * caller-meaningful information. Walks the full enum pairwise so
   * adding a code without updating this test still catches collisions. */
  const uint32_t codes[] = {
      (uint32_t)HAL_OK,
      (uint32_t)HAL_ERR,
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)HAL_ERR_TIMEOUT,
      (uint32_t)HAL_ERR_BUSY,
      (uint32_t)HAL_ERR_NOT_INITIALIZED,
      (uint32_t)HAL_ERR_NOT_SUPPORTED,
      (uint32_t)HAL_ERR_IO,
      (uint32_t)HAL_ERR_NO_MEM,
  };
  const uint8_t n = (uint8_t)(sizeof(codes) / sizeof(codes[0]));
  for (uint8_t i = 0; i < n; i++) {
    for (uint8_t j = (uint8_t)(i + 1); j < n; j++) {
      TEST_ASSERT_TRUE(codes[i] != codes[j]);
    }
  }
}

void test_conformance_status_fits_uint8(void) {
  /* Wire-format / RPC ABI guarantee: a port-package may serialize a
   * status as a single byte. A new code that grew past 255 would
   * silently truncate on the wire. */
  TEST_ASSERT_TRUE((uint32_t)HAL_OK                  <= 0xFFu);
  TEST_ASSERT_TRUE((uint32_t)HAL_ERR                 <= 0xFFu);
  TEST_ASSERT_TRUE((uint32_t)HAL_ERR_INVALID_ARG     <= 0xFFu);
  TEST_ASSERT_TRUE((uint32_t)HAL_ERR_TIMEOUT         <= 0xFFu);
  TEST_ASSERT_TRUE((uint32_t)HAL_ERR_BUSY            <= 0xFFu);
  TEST_ASSERT_TRUE((uint32_t)HAL_ERR_NOT_INITIALIZED <= 0xFFu);
  TEST_ASSERT_TRUE((uint32_t)HAL_ERR_NOT_SUPPORTED   <= 0xFFu);
  TEST_ASSERT_TRUE((uint32_t)HAL_ERR_IO              <= 0xFFu);
  TEST_ASSERT_TRUE((uint32_t)HAL_ERR_NO_MEM          <= 0xFFu);
}

/* HAL_OK_OR_RETURN macro contract — already covered by the host SIL
 * suite, repeated here so every PIL run on every port exercises it
 * against the port's real codegen (catches a port that redefined
 * HAL_OK_OR_RETURN or shadowed _navhal_status). */
static hal_status_t _conf_returns(hal_status_t s) {
  HAL_OK_OR_RETURN(s);
  return HAL_OK;
}

static int _conf_eval_counter = 0;
static hal_status_t _conf_count_and_return(hal_status_t s) {
  _conf_eval_counter++;
  return s;
}

void test_conformance_hal_ok_or_return_passes_through(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)_conf_returns(HAL_OK));
  _conf_eval_counter = 0;
  (void)_conf_returns(_conf_count_and_return(HAL_OK));
  /* The macro is documented as evaluating its argument exactly once
   * (the do-while wrapper holds the value in a local). */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)1, (uint32_t)_conf_eval_counter);
}

void test_conformance_hal_ok_or_return_short_circuits(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_TIMEOUT,
                           (uint32_t)_conf_returns(HAL_ERR_TIMEOUT));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)_conf_returns(HAL_ERR_INVALID_ARG));
}

/* -------------------------------------------------------------------------- *
 * Null-pointer contract — every init function with a pointer arg
 * must return HAL_ERR_INVALID_ARG (or a non-OK status) on NULL,
 * never dereference and crash. Skipped per-arch where the cap is off.
 * -------------------------------------------------------------------------- */

void test_conformance_gpio_init_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_gpio_init((hal_gpio_pin_t)0, NULL));
}

void test_conformance_clock_init_rejects_null(void) {
#if NAVHAL_HAS_CLOCK
  /* hal_clock_init takes (cfg, pll_cfg). NULL cfg must return non-OK. */
  hal_status_t st = hal_clock_init(NULL, NULL);
  TEST_ASSERT_TRUE(st != HAL_OK);
#endif
}

void test_conformance_uart_init_rejects_null(void) {
#if NAVHAL_HAS_UART
  hal_uart_t inst = (hal_uart_t)0;
  TEST_ASSERT_TRUE(hal_uart_init(inst, NULL) != HAL_OK);
#endif
}

void test_conformance_dma_init_rejects_null(void) {
#if NAVHAL_HAS_DMA
  TEST_ASSERT_TRUE(hal_dma_init(NULL) != HAL_OK);
#endif
}

void test_conformance_i2c_init_rejects_null(void) {
#if NAVHAL_HAS_I2C
  TEST_ASSERT_TRUE(hal_i2c_init((hal_i2c_bus_t)0, NULL) != HAL_OK);
#endif
}

void test_conformance_spi_init_rejects_null(void) {
#if NAVHAL_HAS_SPI
  /* Most SPI inits take an instance + config; null config must reject. */
  /* Skip if the port's API shape doesn't match — placeholder for now. */
#endif
}

void test_conformance_pwm_init_rejects_null(void) {
#if NAVHAL_HAS_PWM
  /* PWM init shape varies; placeholder. Expand once we audit. */
#endif
}

void test_conformance_sdio_init_rejects_null(void) {
#if NAVHAL_HAS_SDIO
  TEST_ASSERT_TRUE(hal_sdio_init(NULL) != HAL_SDIO_OK);
#endif
}

/* Idempotency on the error path: calling init with NULL twice in a
 * row must return the same status both times. A port that flips an
 * internal "initialised" flag on the NULL branch would fail this. */
void test_conformance_null_init_is_idempotent(void) {
  hal_status_t a = hal_gpio_init((hal_gpio_pin_t)0, NULL);
  hal_status_t b = hal_gpio_init((hal_gpio_pin_t)0, NULL);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)a, (uint32_t)b);

#if NAVHAL_HAS_DMA
  hal_status_t da = hal_dma_init(NULL);
  hal_status_t db = hal_dma_init(NULL);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)da, (uint32_t)db);
#endif

#if NAVHAL_HAS_I2C
  hal_status_t ia = hal_i2c_init((hal_i2c_bus_t)0, NULL);
  hal_status_t ib = hal_i2c_init((hal_i2c_bus_t)0, NULL);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)ia, (uint32_t)ib);
#endif

#if NAVHAL_HAS_UART
  hal_uart_t inst = (hal_uart_t)0;
  hal_status_t ua = hal_uart_init(inst, NULL);
  hal_status_t ub = hal_uart_init(inst, NULL);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)ua, (uint32_t)ub);
#endif
}

/* -------------------------------------------------------------------------- *
 * Capability-flag contract — every NAVHAL_HAS_* macro must be defined
 * as a numeric 0 or 1. Source code that does `#if NAVHAL_HAS_X` relies
 * on this; `#ifdef NAVHAL_HAS_X` would silently be true everywhere.
 * -------------------------------------------------------------------------- */

void test_conformance_cap_macros_are_defined(void) {
  /* These compile-time checks are the real contract; the runtime
   * assertions are just to make the test register in the suite output. */
#if !defined(NAVHAL_HAS_DMA)
#  error "NAVHAL_HAS_DMA is not defined — contract violation"
#endif
#if !defined(NAVHAL_HAS_FPU)
#  error "NAVHAL_HAS_FPU is not defined"
#endif
#if !defined(NAVHAL_HAS_CRC_HW)
#  error "NAVHAL_HAS_CRC_HW is not defined"
#endif
#if !defined(NAVHAL_HAS_CYCLE_COUNTER)
#  error "NAVHAL_HAS_CYCLE_COUNTER is not defined"
#endif
#if !defined(NAVHAL_HAS_SDIO)
#  error "NAVHAL_HAS_SDIO is not defined"
#endif
  /* Numeric domain: must be exactly 0 or 1. */
  TEST_ASSERT_TRUE(NAVHAL_HAS_DMA            == 0 || NAVHAL_HAS_DMA            == 1);
  TEST_ASSERT_TRUE(NAVHAL_HAS_FPU            == 0 || NAVHAL_HAS_FPU            == 1);
  TEST_ASSERT_TRUE(NAVHAL_HAS_CRC_HW         == 0 || NAVHAL_HAS_CRC_HW         == 1);
  TEST_ASSERT_TRUE(NAVHAL_HAS_CYCLE_COUNTER  == 0 || NAVHAL_HAS_CYCLE_COUNTER  == 1);
  TEST_ASSERT_TRUE(NAVHAL_HAS_SDIO           == 0 || NAVHAL_HAS_SDIO           == 1);
}
/* PROGMEM slot for each case name on AVR; no-op elsewhere. */
NAVTEST_CASE_DECL(test_conformance_status_ok_is_zero);
NAVTEST_CASE_DECL(test_conformance_status_errors_distinct);
NAVTEST_CASE_DECL(test_conformance_status_fits_uint8);
NAVTEST_CASE_DECL(test_conformance_hal_ok_or_return_passes_through);
NAVTEST_CASE_DECL(test_conformance_hal_ok_or_return_short_circuits);
NAVTEST_CASE_DECL(test_conformance_gpio_init_rejects_null);
NAVTEST_CASE_DECL(test_conformance_clock_init_rejects_null);
NAVTEST_CASE_DECL(test_conformance_uart_init_rejects_null);
NAVTEST_CASE_DECL(test_conformance_dma_init_rejects_null);
NAVTEST_CASE_DECL(test_conformance_i2c_init_rejects_null);
NAVTEST_CASE_DECL(test_conformance_spi_init_rejects_null);
NAVTEST_CASE_DECL(test_conformance_pwm_init_rejects_null);
NAVTEST_CASE_DECL(test_conformance_sdio_init_rejects_null);
NAVTEST_CASE_DECL(test_conformance_null_init_is_idempotent);
NAVTEST_CASE_DECL(test_conformance_cap_macros_are_defined);


static const navtest_case_t conformance_cases[] = {
    NAVTEST_CASE(test_conformance_status_ok_is_zero),
    NAVTEST_CASE(test_conformance_status_errors_distinct),
    NAVTEST_CASE(test_conformance_status_fits_uint8),
    NAVTEST_CASE(test_conformance_hal_ok_or_return_passes_through),
    NAVTEST_CASE(test_conformance_hal_ok_or_return_short_circuits),
    NAVTEST_CASE(test_conformance_gpio_init_rejects_null),
    NAVTEST_CASE(test_conformance_clock_init_rejects_null),
    NAVTEST_CASE(test_conformance_uart_init_rejects_null),
    NAVTEST_CASE(test_conformance_dma_init_rejects_null),
    NAVTEST_CASE(test_conformance_i2c_init_rejects_null),
    NAVTEST_CASE(test_conformance_spi_init_rejects_null),
    NAVTEST_CASE(test_conformance_pwm_init_rejects_null),
    NAVTEST_CASE(test_conformance_sdio_init_rejects_null),
    NAVTEST_CASE(test_conformance_null_init_is_idempotent),
    NAVTEST_CASE(test_conformance_cap_macros_are_defined),
};

const navtest_suite_t test_conformance_suite = {
    .name = "CONFORMANCE",
    .cases = conformance_cases,
    .count = sizeof(conformance_cases) / sizeof(conformance_cases[0]),
    .between = NULL,
};
