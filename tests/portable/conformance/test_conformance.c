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
   * caller-meaningful information. */
  TEST_ASSERT_TRUE((uint32_t)HAL_ERR_INVALID_ARG    != (uint32_t)HAL_OK);
  TEST_ASSERT_TRUE((uint32_t)HAL_ERR_TIMEOUT        != (uint32_t)HAL_OK);
  TEST_ASSERT_TRUE((uint32_t)HAL_ERR_NOT_SUPPORTED  != (uint32_t)HAL_OK);
  TEST_ASSERT_TRUE((uint32_t)HAL_ERR_INVALID_ARG    != (uint32_t)HAL_ERR_TIMEOUT);
  TEST_ASSERT_TRUE((uint32_t)HAL_ERR_INVALID_ARG    != (uint32_t)HAL_ERR_NOT_SUPPORTED);
  TEST_ASSERT_TRUE((uint32_t)HAL_ERR_TIMEOUT        != (uint32_t)HAL_ERR_NOT_SUPPORTED);
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

static const navtest_case_t conformance_cases[] = {
    NAVTEST_CASE(test_conformance_status_ok_is_zero),
    NAVTEST_CASE(test_conformance_status_errors_distinct),
    NAVTEST_CASE(test_conformance_gpio_init_rejects_null),
    NAVTEST_CASE(test_conformance_clock_init_rejects_null),
    NAVTEST_CASE(test_conformance_uart_init_rejects_null),
    NAVTEST_CASE(test_conformance_dma_init_rejects_null),
    NAVTEST_CASE(test_conformance_i2c_init_rejects_null),
    NAVTEST_CASE(test_conformance_spi_init_rejects_null),
    NAVTEST_CASE(test_conformance_pwm_init_rejects_null),
    NAVTEST_CASE(test_conformance_sdio_init_rejects_null),
    NAVTEST_CASE(test_conformance_cap_macros_are_defined),
};

const navtest_suite_t test_conformance_suite = {
    .name = "CONFORMANCE",
    .cases = conformance_cases,
    .count = sizeof(conformance_cases) / sizeof(conformance_cases[0]),
    .between = NULL,
};
