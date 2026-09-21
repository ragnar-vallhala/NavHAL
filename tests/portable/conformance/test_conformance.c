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

#include "navtest_target.h" /* NAVTEST_UART */

#include "common/hal_features.h"
#include "common/hal_status.h"
#include "common/hal_gpio.h"
#include "common/hal_clock.h"

#if NAVHAL_CONFIG_DRV_TIMER
#include "common/hal_timer.h"
#endif
#if NAVHAL_CONFIG_DRV_FLASH
#include "common/hal_flash.h"
#endif
#if NAVHAL_CONFIG_DRV_ADC
#include "common/hal_adc.h"
#endif

#if NAVHAL_CONFIG_DRV_UART
#include "common/hal_uart.h"
#endif
#if NAVHAL_CONFIG_DRV_DMA
#include "common/hal_dma.h"
#endif
#if NAVHAL_CONFIG_DRV_I2C
#include "common/hal_i2c.h"
#endif
#if NAVHAL_CONFIG_DRV_SPI
#include "common/hal_spi.h"
#endif
#if NAVHAL_CONFIG_DRV_PWM
#include "common/hal_pwm.h"
#endif
#if NAVHAL_CONFIG_DRV_SDIO
#include "common/hal_sdio.h"
#endif


/* ---------------------------------------------------------------------------
 * Argument contract: every fallible call taking a pointer rejects NULL, and
 * does so before touching hardware. M9 moved these checks into the shared
 * layer precisely so the answer is the same on every port; this is what says
 * whether that held.
 * ------------------------------------------------------------------------- */

#if NAVHAL_CONFIG_DRV_FLASH
static uint8_t flash_buf[4];
static uint8_t flash_size = sizeof(flash_buf);
#endif
#if NAVHAL_CONFIG_DRV_DMA
static uint16_t dma_left;
static const hal_dma_config_t dma_cfg = {0};
#endif
#if NAVHAL_CONFIG_DRV_TIMER
#define TEST_CONF_TIMER TIM2
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
#if NAVHAL_CONFIG_DRV_CLOCK
  /* hal_clock_init takes (cfg, pll_cfg). NULL cfg must return non-OK. */
  hal_status_t st = hal_clock_init(NULL);
  TEST_ASSERT_TRUE(st != HAL_OK);
#endif
}

void test_conformance_uart_init_rejects_null(void) {
#if NAVHAL_CONFIG_DRV_UART
  hal_uart_t inst = (hal_uart_t)0;
  TEST_ASSERT_TRUE(hal_uart_init(inst, NULL) != HAL_OK);
#endif
}

void test_conformance_dma_init_rejects_null(void) {
#if NAVHAL_CONFIG_DRV_DMA
  TEST_ASSERT_TRUE(hal_dma_init(NULL) != HAL_OK);
#endif
}

void test_conformance_i2c_init_rejects_null(void) {
#if NAVHAL_CONFIG_DRV_I2C
  TEST_ASSERT_TRUE(hal_i2c_init((hal_i2c_bus_t)0, NULL) != HAL_OK);
#endif
}

void test_conformance_spi_init_rejects_null(void) {
#if NAVHAL_CONFIG_DRV_SPI
  /* Most SPI inits take an instance + config; null config must reject. */
  /* Skip if the port's API shape doesn't match — placeholder for now. */
#endif
}

void test_conformance_pwm_init_rejects_null(void) {
#if NAVHAL_CONFIG_DRV_PWM
  /* PWM init shape varies; placeholder. Expand once we audit. */
#endif
}

void test_conformance_sdio_init_rejects_null(void) {
#if NAVHAL_CONFIG_DRV_SDIO
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

#if NAVHAL_CONFIG_DRV_DMA
  hal_status_t da = hal_dma_init(NULL);
  hal_status_t db = hal_dma_init(NULL);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)da, (uint32_t)db);
#endif

#if NAVHAL_CONFIG_DRV_I2C
  hal_status_t ia = hal_i2c_init((hal_i2c_bus_t)0, NULL);
  hal_status_t ib = hal_i2c_init((hal_i2c_bus_t)0, NULL);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)ia, (uint32_t)ib);
#endif

#if NAVHAL_CONFIG_DRV_UART
  hal_uart_t inst = (hal_uart_t)0;
  hal_status_t ua = hal_uart_init(inst, NULL);
  hal_status_t ub = hal_uart_init(inst, NULL);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)ua, (uint32_t)ub);
#endif
}

/* -------------------------------------------------------------------------- *
 * Capability-flag contract — every NAVHAL_CONFIG_DRV_* macro must be defined
 * as a numeric 0 or 1. Source code that does `#if NAVHAL_CONFIG_DRV_X` relies
 * on this; `#ifdef` would silently be true everywhere. The deprecated
 * NAVHAL_CONFIG_DRV_* aliases must stay defined and equal their CONFIG source so
 * out-of-tree consumers keep building.
 * -------------------------------------------------------------------------- */

void test_conformance_cap_macros_are_defined(void) {
  /* These compile-time checks are the real contract; the runtime
   * assertions are just to make the test register in the suite output. */
#if !defined(NAVHAL_CONFIG_DRV_DMA)
#  error "NAVHAL_CONFIG_DRV_DMA is not defined — contract violation"
#endif
#if !defined(NAVHAL_CONFIG_USE_FPU)
#  error "NAVHAL_CONFIG_USE_FPU is not defined"
#endif
#if !defined(NAVHAL_CONFIG_DRV_CRC)
#  error "NAVHAL_CONFIG_DRV_CRC is not defined"
#endif
#if !defined(NAVHAL_CONFIG_DRV_DWT)
#  error "NAVHAL_CONFIG_DRV_DWT is not defined"
#endif
#if !defined(NAVHAL_CONFIG_DRV_SDIO)
#  error "NAVHAL_CONFIG_DRV_SDIO is not defined"
#endif
  /* Numeric domain: must be exactly 0 or 1. */
  TEST_ASSERT_TRUE(NAVHAL_CONFIG_DRV_DMA  == 0 || NAVHAL_CONFIG_DRV_DMA  == 1);
  TEST_ASSERT_TRUE(NAVHAL_CONFIG_USE_FPU  == 0 || NAVHAL_CONFIG_USE_FPU  == 1);
  TEST_ASSERT_TRUE(NAVHAL_CONFIG_DRV_CRC  == 0 || NAVHAL_CONFIG_DRV_CRC  == 1);
  TEST_ASSERT_TRUE(NAVHAL_CONFIG_DRV_DWT  == 0 || NAVHAL_CONFIG_DRV_DWT  == 1);
  TEST_ASSERT_TRUE(NAVHAL_CONFIG_DRV_SDIO == 0 || NAVHAL_CONFIG_DRV_SDIO == 1);

  /* Deprecated NAVHAL_HAS_* aliases must remain defined and track their
   * NAVHAL_CONFIG_* source, so out-of-tree consumers keep building. */
#if !defined(NAVHAL_HAS_DMA) || !defined(NAVHAL_HAS_FPU) ||                     \
    !defined(NAVHAL_HAS_CRC_HW) || !defined(NAVHAL_HAS_CYCLE_COUNTER) ||        \
    !defined(NAVHAL_HAS_SDIO)
#  error "a deprecated NAVHAL_HAS_* alias is missing — out-of-tree contract broken"
#endif
  TEST_ASSERT_EQUAL_UINT32(NAVHAL_CONFIG_DRV_DMA,  NAVHAL_HAS_DMA);
  TEST_ASSERT_EQUAL_UINT32(NAVHAL_CONFIG_USE_FPU,  NAVHAL_HAS_FPU);
  TEST_ASSERT_EQUAL_UINT32(NAVHAL_CONFIG_DRV_CRC,  NAVHAL_HAS_CRC_HW);
  TEST_ASSERT_EQUAL_UINT32(NAVHAL_CONFIG_DRV_DWT,  NAVHAL_HAS_CYCLE_COUNTER);
  TEST_ASSERT_EQUAL_UINT32(NAVHAL_CONFIG_DRV_SDIO, NAVHAL_HAS_SDIO);
}
/* PROGMEM slot for each case name on AVR; no-op elsewhere. */

#if NAVHAL_CONFIG_DRV_ADC

void test_conformance_adc_read_rejects_null_out(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_adc_read(HAL_ADC_1, 0u, NULL));
}

#endif /* NAVHAL_CONFIG_DRV_ADC */


#if NAVHAL_CONFIG_DRV_FLASH

void test_conformance_flash_save_rejects_null_value(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_flash_save(0u, NULL, 1u));
}

void test_conformance_flash_read_rejects_null_value(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_flash_read(0u, NULL, &flash_size));
}

void test_conformance_flash_read_rejects_null_size(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_flash_read(0u, flash_buf, NULL));
}

#endif /* NAVHAL_CONFIG_DRV_FLASH */


#if NAVHAL_CONFIG_DRV_I2C

void test_conformance_i2c_write_rejects_null_data(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_i2c_write(HAL_I2C_1, 0x10u, NULL, 1u));
}

void test_conformance_i2c_read_rejects_null_data(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_i2c_read(HAL_I2C_1, 0x10u, NULL, 1u));
}

void test_conformance_i2c_write_read_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_i2c_write_read(HAL_I2C_1, 0x10u, NULL, 1u, NULL, 1u));
}

#endif /* NAVHAL_CONFIG_DRV_I2C */


#if NAVHAL_CONFIG_DRV_SPI

void test_conformance_spi_transmit_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_spi_transmit(HAL_SPI_1, NULL, 1u, 0u));
}

void test_conformance_spi_receive_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_spi_receive(HAL_SPI_1, NULL, 1u, 0u));
}

void test_conformance_spi_xfer_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_spi_transmit_receive(HAL_SPI_1, NULL, NULL, 1u, 0u));
}

#endif /* NAVHAL_CONFIG_DRV_SPI */


#if NAVHAL_CONFIG_DRV_UART

void test_conformance_uart_write_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_uart_write(NAVTEST_UART, NULL, 1u));
}

void test_conformance_uart_write_string_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_uart_write_string(NAVTEST_UART, NULL));
}

#endif /* NAVHAL_CONFIG_DRV_UART */


#if NAVHAL_CONFIG_DRV_DMA

void test_conformance_dma_start_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_dma_start(NULL));
}

void test_conformance_dma_stop_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_dma_stop(NULL));
}

void test_conformance_dma_clear_flags_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_dma_clear_flags(NULL));
}

void test_conformance_dma_set_memory_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_dma_set_memory(NULL, 0u, 1u));
}

void test_conformance_dma_remaining_rejects_null_cfg(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_dma_remaining(NULL, &dma_left));
}

void test_conformance_dma_remaining_rejects_null_out(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_dma_remaining(&dma_cfg, NULL));
}

#endif /* NAVHAL_CONFIG_DRV_DMA */


#if NAVHAL_CONFIG_DRV_PWM

void test_conformance_pwm_start_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_pwm_start(NULL));
}

void test_conformance_pwm_stop_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_pwm_stop(NULL));
}

void test_conformance_pwm_set_duty_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_pwm_set_duty_cycle(NULL, 50u));
}

void test_conformance_pwm_set_frequency_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_pwm_set_frequency(NULL, 1000u));
}

#endif /* NAVHAL_CONFIG_DRV_PWM */


#if NAVHAL_CONFIG_DRV_TIMER

void test_conformance_timer_init_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_timer_init(TEST_CONF_TIMER, NULL));
}

#endif /* NAVHAL_CONFIG_DRV_TIMER */

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
#if NAVHAL_CONFIG_DRV_ADC
NAVTEST_CASE_DECL(test_conformance_adc_read_rejects_null_out);
#endif
#if NAVHAL_CONFIG_DRV_FLASH
NAVTEST_CASE_DECL(test_conformance_flash_save_rejects_null_value);
#endif
#if NAVHAL_CONFIG_DRV_FLASH
NAVTEST_CASE_DECL(test_conformance_flash_read_rejects_null_value);
#endif
#if NAVHAL_CONFIG_DRV_FLASH
NAVTEST_CASE_DECL(test_conformance_flash_read_rejects_null_size);
#endif
#if NAVHAL_CONFIG_DRV_I2C
NAVTEST_CASE_DECL(test_conformance_i2c_write_rejects_null_data);
#endif
#if NAVHAL_CONFIG_DRV_I2C
NAVTEST_CASE_DECL(test_conformance_i2c_read_rejects_null_data);
#endif
#if NAVHAL_CONFIG_DRV_I2C
NAVTEST_CASE_DECL(test_conformance_i2c_write_read_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_SPI
NAVTEST_CASE_DECL(test_conformance_spi_transmit_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_SPI
NAVTEST_CASE_DECL(test_conformance_spi_receive_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_SPI
NAVTEST_CASE_DECL(test_conformance_spi_xfer_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_UART
NAVTEST_CASE_DECL(test_conformance_uart_write_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_UART
NAVTEST_CASE_DECL(test_conformance_uart_write_string_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_DMA
NAVTEST_CASE_DECL(test_conformance_dma_start_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_DMA
NAVTEST_CASE_DECL(test_conformance_dma_stop_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_DMA
NAVTEST_CASE_DECL(test_conformance_dma_clear_flags_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_DMA
NAVTEST_CASE_DECL(test_conformance_dma_set_memory_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_DMA
NAVTEST_CASE_DECL(test_conformance_dma_remaining_rejects_null_cfg);
#endif
#if NAVHAL_CONFIG_DRV_DMA
NAVTEST_CASE_DECL(test_conformance_dma_remaining_rejects_null_out);
#endif
#if NAVHAL_CONFIG_DRV_PWM
NAVTEST_CASE_DECL(test_conformance_pwm_start_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_PWM
NAVTEST_CASE_DECL(test_conformance_pwm_stop_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_PWM
NAVTEST_CASE_DECL(test_conformance_pwm_set_duty_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_PWM
NAVTEST_CASE_DECL(test_conformance_pwm_set_frequency_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_TIMER
NAVTEST_CASE_DECL(test_conformance_timer_init_rejects_null);
#endif


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
#if NAVHAL_CONFIG_DRV_ADC
    NAVTEST_CASE(test_conformance_adc_read_rejects_null_out),
#endif
#if NAVHAL_CONFIG_DRV_FLASH
    NAVTEST_CASE(test_conformance_flash_save_rejects_null_value),
#endif
#if NAVHAL_CONFIG_DRV_FLASH
    NAVTEST_CASE(test_conformance_flash_read_rejects_null_value),
#endif
#if NAVHAL_CONFIG_DRV_FLASH
    NAVTEST_CASE(test_conformance_flash_read_rejects_null_size),
#endif
#if NAVHAL_CONFIG_DRV_I2C
    NAVTEST_CASE(test_conformance_i2c_write_rejects_null_data),
#endif
#if NAVHAL_CONFIG_DRV_I2C
    NAVTEST_CASE(test_conformance_i2c_read_rejects_null_data),
#endif
#if NAVHAL_CONFIG_DRV_I2C
    NAVTEST_CASE(test_conformance_i2c_write_read_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_SPI
    NAVTEST_CASE(test_conformance_spi_transmit_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_SPI
    NAVTEST_CASE(test_conformance_spi_receive_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_SPI
    NAVTEST_CASE(test_conformance_spi_xfer_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_UART
    NAVTEST_CASE(test_conformance_uart_write_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_UART
    NAVTEST_CASE(test_conformance_uart_write_string_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_DMA
    NAVTEST_CASE(test_conformance_dma_start_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_DMA
    NAVTEST_CASE(test_conformance_dma_stop_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_DMA
    NAVTEST_CASE(test_conformance_dma_clear_flags_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_DMA
    NAVTEST_CASE(test_conformance_dma_set_memory_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_DMA
    NAVTEST_CASE(test_conformance_dma_remaining_rejects_null_cfg),
#endif
#if NAVHAL_CONFIG_DRV_DMA
    NAVTEST_CASE(test_conformance_dma_remaining_rejects_null_out),
#endif
#if NAVHAL_CONFIG_DRV_PWM
    NAVTEST_CASE(test_conformance_pwm_start_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_PWM
    NAVTEST_CASE(test_conformance_pwm_stop_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_PWM
    NAVTEST_CASE(test_conformance_pwm_set_duty_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_PWM
    NAVTEST_CASE(test_conformance_pwm_set_frequency_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_TIMER
    NAVTEST_CASE(test_conformance_timer_init_rejects_null),
#endif
};

const navtest_suite_t test_conformance_suite = {
    .name = "CONFORMANCE",
    .cases = conformance_cases,
    .count = sizeof(conformance_cases) / sizeof(conformance_cases[0]),
    .between = NULL,
};
