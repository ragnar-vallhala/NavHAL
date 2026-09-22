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
#if NAVHAL_CONFIG_DRV_GPIO
#include "board.h" /* LED_BUILTIN: a pin the harness may reconfigure */
#endif

#include "common/hal_features.h"
#include "common/hal_status.h"
#include "common/hal_gpio.h"
#include "common/hal_clock.h"
#if NAVHAL_CONFIG_DRV_RESET
#include "common/hal_reset.h"
#endif
#if NAVHAL_CONFIG_USE_FPU
#include "common/hal_fpu.h"
#endif
#if NAVHAL_CONFIG_DRV_MPU
#include "common/hal_mpu.h"
#endif
#if NAVHAL_CONFIG_DRV_DWT
#include "common/hal_dwt.h"
#endif
#if NAVHAL_CONFIG_DRV_CACHE
#include "common/hal_cache.h"
#endif
#if NAVHAL_CONFIG_DRV_WATCHDOG
#include "common/hal_watchdog.h"
#endif
#if NAVHAL_CONFIG_DRV_USB_CDC
#include "common/hal_usb_cdc.h"
#endif
#if NAVHAL_CONFIG_DRV_RTC
#include "common/hal_rtc.h"
#endif
#if NAVHAL_CONFIG_DRV_ETH
#include "common/hal_eth.h"
#endif

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
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_spi_init(HAL_SPI_1, NULL));
#endif
}

void test_conformance_pwm_init_rejects_null(void) {
#if NAVHAL_CONFIG_DRV_PWM
  /* Every entry point takes the caller's handle, so every one of them has to
   * survive being handed nothing. */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_pwm_init(NULL, 1000u, 0.5f));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_pwm_start(NULL));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_pwm_stop(NULL));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_pwm_set_duty_cycle(NULL, 0.5f));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_pwm_set_frequency(NULL, 1000u));
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


/* Valid configs, so an instance-id test fails for the reason it says rather
 * than tripping the NULL check first. */
#if NAVHAL_CONFIG_DRV_I2C
static const hal_i2c_config_t conf_i2c_cfg = {.clock_speed = HAL_I2C_SPEED_STANDARD,
                                              .own_address = I2C_MASTER,
                                              .acknowledge = true};
#endif
#if NAVHAL_CONFIG_DRV_SPI
static const hal_spi_config_t conf_spi_cfg = {.baudrate = HAL_SPI_BAUDRATE_DIV8,
                                              .cpol = HAL_SPI_CPOL_LOW,
                                              .cpha = HAL_SPI_CPHA_1EDGE,
                                              .datasize = HAL_SPI_DATASIZE_8BIT,
                                              .firstbit = HAL_SPI_FIRSTBIT_MSB};
#endif
#if NAVHAL_CONFIG_DRV_UART
static const hal_uart_config_t conf_uart_cfg = {.baudrate = 9600u};
#endif

/* An instance the port does not have is rejected, not poked. Only the backend
 * knows its own valid range, so this is the half of the argument contract
 * that deliberately did not move into the shared layer. */

#if NAVHAL_CONFIG_DRV_ADC

void test_conformance_adc_rejects_bad_unit(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_adc_init((hal_adc_t)99, NULL));
}

#endif /* NAVHAL_CONFIG_DRV_ADC */


#if NAVHAL_CONFIG_DRV_I2C

void test_conformance_i2c_init_rejects_bad_bus(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_i2c_init((hal_i2c_bus_t)99, &conf_i2c_cfg));
}

void test_conformance_i2c_deinit_rejects_bad_bus(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_i2c_deinit((hal_i2c_bus_t)99));
}

#endif /* NAVHAL_CONFIG_DRV_I2C */


#if NAVHAL_CONFIG_DRV_SPI

void test_conformance_spi_init_rejects_bad_instance(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_spi_init((hal_spi_instance_t)99, &conf_spi_cfg));
}

#endif /* NAVHAL_CONFIG_DRV_SPI */


#if NAVHAL_CONFIG_DRV_TIMER

void test_conformance_timer_start_rejects_bad_timer(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_timer_start((hal_timer_t)99));
}

void test_conformance_timer_stop_rejects_bad_timer(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_timer_stop((hal_timer_t)99));
}

void test_conformance_timer_reset_rejects_bad_timer(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_timer_reset((hal_timer_t)99));
}

void test_conformance_timer_set_divider_rejects_zero(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_timer_set_divider(TEST_CONF_TIMER, 0u));
}

#endif /* NAVHAL_CONFIG_DRV_TIMER */


#if NAVHAL_CONFIG_DRV_UART

void test_conformance_uart_init_rejects_bad_instance(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_uart_init((hal_uart_t)99, &conf_uart_cfg));
}

#endif /* NAVHAL_CONFIG_DRV_UART */


/* ---------------------------------------------------------------------------
 * Core/arch features: getters agree with the state they report, and the
 * enable/disable pairs round-trip. Safe on a bare board -- none of this needs
 * anything attached.
 * ------------------------------------------------------------------------- */

#if NAVHAL_CONFIG_DRV_CACHE
static uint8_t cache_buf[64] NAVHAL_DMA_ALIGN;

void test_conformance_icache_enable_round_trips(void) {
  hal_icache_enable();
  TEST_ASSERT_TRUE(hal_icache_is_enabled());
  hal_icache_disable();
  TEST_ASSERT_TRUE(!hal_icache_is_enabled());
  hal_icache_enable(); /* leave it as the harness found it */
}

void test_conformance_dcache_enable_round_trips(void) {
  hal_dcache_enable();
  TEST_ASSERT_TRUE(hal_dcache_is_enabled());
  /* Maintenance on an enabled cache must not fault on a valid range. */
  hal_dcache_clean(cache_buf, sizeof(cache_buf));
  hal_dcache_invalidate(cache_buf, sizeof(cache_buf));
  hal_dcache_clean_invalidate(cache_buf, sizeof(cache_buf));
  hal_dcache_disable();
  TEST_ASSERT_TRUE(!hal_dcache_is_enabled());
  hal_dcache_enable();
}
#endif /* NAVHAL_CONFIG_DRV_CACHE */

#if NAVHAL_CONFIG_DRV_CLOCK
void test_conformance_clock_getters_are_consistent(void) {
  uint32_t sysclk = hal_clock_get_sysclk();
  TEST_ASSERT_TRUE(sysclk > 0u);

  /* A bus is never faster than the clock it divides. */
  uint8_t n = hal_clock_get_bus_count();
  for (uint8_t i = 0; i < n; i++) {
    uint32_t bus = hal_clock_get_bus_clock(i);
    TEST_ASSERT_TRUE(bus > 0u);
    TEST_ASSERT_TRUE(bus <= sysclk);
  }
}

void test_conformance_clock_rejects_unknown_bus(void) {
  /* Index past the reported count is not a bus, whatever the port. */
  TEST_ASSERT_EQUAL_UINT32(0u, hal_clock_get_bus_clock(hal_clock_get_bus_count()));
  TEST_ASSERT_EQUAL_UINT32(0u, hal_clock_get_bus_clock(200u));
}
#endif /* NAVHAL_CONFIG_DRV_CLOCK */

#if NAVHAL_CONFIG_DRV_CRC
void test_conformance_crc_reset_restarts_accumulation(void) {
  static const uint8_t vec[4] = {0xDE, 0xAD, 0xBE, 0xEF};

  uint32_t once = hal_crc_compute(vec, sizeof(vec));

  /* Accumulating the same bytes after a reset must land in the same place:
   * compute is defined as reset-then-accumulate. */
  hal_crc_reset();
  uint32_t again = hal_crc_accumulate(vec, sizeof(vec));
  TEST_ASSERT_EQUAL_UINT32(once, again);

  /* And it is a checksum, not a constant. */
  static const uint8_t other[4] = {0x00, 0x00, 0x00, 0x00};
  TEST_ASSERT_TRUE(hal_crc_compute(other, sizeof(other)) != once);
}
#endif /* NAVHAL_CONFIG_DRV_CRC */

#if NAVHAL_CONFIG_DRV_DMA
void test_conformance_dma_transfer_complete_rejects_null(void) {
  TEST_ASSERT_TRUE(!hal_dma_transfer_complete(NULL));
}
#endif

#if NAVHAL_CONFIG_DRV_DWT
void test_conformance_dwt_counter_advances(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_cycle_counter_init());
  TEST_ASSERT_TRUE(hal_cycle_counter_cycles_per_us() > 0u);

  hal_cycle_counter_reset();
  uint32_t start = hal_cycle_counter_get();
  hal_cycle_counter_delay_us(10u);
  TEST_ASSERT_TRUE(hal_cycle_counter_get() > start);

  /* The microsecond view has to agree that time passed. */
  uint32_t us0 = hal_cycle_counter_get_us();
  hal_cycle_counter_delay(hal_cycle_counter_cycles_per_us() * 10u);
  TEST_ASSERT_TRUE(hal_cycle_counter_get_us() >= us0);
}
#endif /* NAVHAL_CONFIG_DRV_DWT */

#if NAVHAL_CONFIG_USE_FPU
void test_conformance_fpu_enable_is_idempotent(void) {
  hal_fpu_enable();
  hal_fpu_enable(); /* enabling an enabled FPU is not an error */
}
#endif

#if NAVHAL_CONFIG_DRV_GPIO
void test_conformance_gpio_mode_round_trips(void) {
  /* The board LED: an output the harness is free to reconfigure. */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_gpio_enable_clock(LED_BUILTIN));
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_OK,
      (uint32_t)hal_gpio_set_mode(LED_BUILTIN, HAL_GPIO_MODE_OUTPUT,
                                  HAL_GPIO_PULL_NONE));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_GPIO_MODE_OUTPUT,
                           (uint32_t)hal_gpio_get_mode(LED_BUILTIN));

  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_OK,
      (uint32_t)hal_gpio_set_output_type(LED_BUILTIN, HAL_GPIO_OTYPE_PUSH_PULL));
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_OK,
      (uint32_t)hal_gpio_set_output_speed(LED_BUILTIN, HAL_GPIO_SPEED_LOW));
}

void test_conformance_gpio_alternate_function_keeps_mode(void) {
  /* Selecting an AF must leave the pin in AF mode -- the pull is the caller's,
   * and clobbering it here was a real bug once. */
  hal_status_t af = hal_gpio_set_alternate_function(LED_BUILTIN,
                                                    (hal_gpio_af_t)0);
  if (af == HAL_ERR_NOT_SUPPORTED) {
    return; /* a port with no pin multiplexer (AVR) */
  }
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)af);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_GPIO_MODE_AF,
                           (uint32_t)hal_gpio_get_mode(LED_BUILTIN));

  /* Put it back so a later suite finds an output. */
  hal_gpio_set_mode(LED_BUILTIN, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
}
#endif /* NAVHAL_CONFIG_DRV_GPIO */

#if NAVHAL_CONFIG_DRV_I2C
void test_conformance_i2c_init_status_is_a_bus_mask(void) {
  /* One bit per bus, so nothing above the bus count may be set. */
  uint8_t st = hal_i2c_get_init_status();
  TEST_ASSERT_EQUAL_UINT32(0u, (uint32_t)(st & (uint8_t)~0x07u));
}
#endif

#if NAVHAL_CONFIG_DRV_MPU
void test_conformance_mpu_reports_its_regions(void) {
  if (!hal_mpu_present()) {
    TEST_ASSERT_EQUAL_UINT32(0u, (uint32_t)hal_mpu_num_regions());
    return;
  }
  TEST_ASSERT_TRUE(hal_mpu_num_regions() > 0u);
}

void test_conformance_mpu_rejects_null(void) {
  hal_mpu_region_t r = {.base = 0x20000000u,
                        .size = HAL_MPU_SIZE_1KB,
                        .ap = HAL_MPU_AP_RW,
                        .mem = HAL_MPU_MEM_NORMAL_WB,
                        .executable = false,
                        .shareable = false,
                        .srd_mask = 0u};
  hal_mpu_encoded_t enc;

  if (!hal_mpu_present()) {
    TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_NOT_SUPPORTED,
                             (uint32_t)hal_mpu_configure_region(0u, &r));
    return;
  }

  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_mpu_configure_region(0u, NULL));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_mpu_encode(0u, &r, NULL));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_mpu_encode(0u, NULL, &enc));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_mpu_apply(NULL, 1u));

  /* Past the last hardware region. */
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_mpu_encode(hal_mpu_num_regions(), &r, &enc));
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_mpu_apply(&enc, hal_mpu_num_regions() + 1u));
}

void test_conformance_mpu_rejects_misaligned_base(void) {
  /* The hardware takes base and size from one register: a base that is not a
   * multiple of its own size silently covers the wrong range, so it has to be
   * refused rather than encoded. */
  hal_mpu_region_t r = {.base = 0x20000000u + 32u, /* 32 B into a 1 KB span */
                        .size = HAL_MPU_SIZE_1KB,
                        .ap = HAL_MPU_AP_RW,
                        .mem = HAL_MPU_MEM_NORMAL_WB,
                        .executable = false,
                        .shareable = false,
                        .srd_mask = 0u};
  hal_mpu_encoded_t enc;

  if (!hal_mpu_present()) {
    return;
  }
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_mpu_encode(0u, &r, &enc));

  /* Aligned, and the encoding must carry the base and the region number it
   * was given -- the two fields a context switch replays verbatim. */
  r.base = 0x20000000u;
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_mpu_encode(1u, &r, &enc));
  TEST_ASSERT_EQUAL_UINT32(0x20000000u, enc.rbar & 0xFFFFFFE0u);
  TEST_ASSERT_EQUAL_UINT32(1u, enc.rbar & 0xFu);
}

void test_conformance_mpu_enable_round_trips(void) {
  if (!hal_mpu_present()) {
    TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_NOT_SUPPORTED,
                             (uint32_t)hal_mpu_enable(true));
    return;
  }
  /* bg_priv = true: with no regions programmed, the privileged background
   * region is what keeps this code fetching its next instruction. */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_mpu_enable(true));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_mpu_disable());
}

#endif /* NAVHAL_CONFIG_DRV_MPU */

#if NAVHAL_CONFIG_DRV_RESET
void test_conformance_reset_reports_a_cause(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_reset_init());
  /* Something reset this part to get here, so at least one flag latched --
   * and latching is idempotent, so a second read agrees. */
  uint32_t cause = hal_reset_get_cause();
  TEST_ASSERT_EQUAL_UINT32(cause, hal_reset_get_cause());
  /* hal_system_reset is deliberately never called: it would end the run. */
}
#endif

#if NAVHAL_CONFIG_DRV_FLASH
void test_conformance_flash_delete_absent_key_is_an_error(void) {
  /* Key 0xFE is not written by any test; deleting it cannot succeed.
   * hal_flash_erase is left alone -- it would wipe the key/value store. */
  TEST_ASSERT_TRUE(hal_flash_delete(0xFEu) != HAL_OK);
  (void)hal_flash_needs_compaction(); /* must not fault */
}
#endif


/* ---------------------------------------------------------------------------
 * Drivers whose hardware is not on a bare board. Only the paths that return
 * before touching it are asserted: nothing here may block waiting for a PHY
 * link, an SD card or USB enumeration, and nothing may arm a watchdog or
 * reset the part, because the suite has to survive its own tests.
 * ------------------------------------------------------------------------- */

#if NAVHAL_CONFIG_DRV_ETH
void test_conformance_eth_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_eth_send(NULL, 1u));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_eth_get_link(NULL));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_eth_set_mac_address(NULL));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_eth_get_mac_address(NULL));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_eth_phy_read(0u, NULL));
  uint16_t got_len = 0u;
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_eth_receive(NULL, 64u, &got_len));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_eth_init(NULL));
  /* init/start/stop are not called: with no PHY attached they wait on a link
   * that never comes up. */
}
#endif /* NAVHAL_CONFIG_DRV_ETH */

#if NAVHAL_CONFIG_DRV_SDIO
void test_conformance_sdio_rejects_null(void) {
  /* This driver predates hal_status_t and reports through its own enum, so
   * the assertion is only that a NULL buffer is not accepted as success. */
  TEST_ASSERT_TRUE(hal_sdio_read_block(0u, NULL) != HAL_SDIO_OK);
  TEST_ASSERT_TRUE(hal_sdio_write_block(0u, NULL) != HAL_SDIO_OK);
  /* No card was initialised, so there is no capacity to report. */
  TEST_ASSERT_EQUAL_UINT32(0u, hal_sdio_get_sector_count());
  /* card_init, wait_flag and wait_sync all block without a card. */
}
#endif /* NAVHAL_CONFIG_DRV_SDIO */

#if NAVHAL_CONFIG_DRV_USB_CDC
void test_conformance_usb_cdc_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_usb_cdc_write(NULL, 1u));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_usb_cdc_write_string(NULL));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_usb_cdc_get_line_coding(NULL));
  /* Unenumerated, so reads return nothing rather than blocking. */
  TEST_ASSERT_EQUAL_UINT32(0u, (uint32_t)hal_usb_cdc_read(NULL, 8u));
}

void test_conformance_usb_cdc_reports_disconnected(void) {
  /* Nothing is plugged into a bare board, so these must say so rather than
   * report a connection that is not there. */
  if (!hal_usb_cdc_connected()) {
    TEST_ASSERT_EQUAL_UINT32(0u, (uint32_t)hal_usb_cdc_available());
  }
}
#endif /* NAVHAL_CONFIG_DRV_USB_CDC */

#if NAVHAL_CONFIG_DRV_WATCHDOG
void test_conformance_watchdog_reports_its_limits(void) {
  /* The ceiling is a hardware property and must be usable as one. */
  uint32_t max_ms = hal_watchdog_max_timeout_ms();
  TEST_ASSERT_TRUE(max_ms > 0u);

  /* Rejected before the peripheral is touched, so the part is not armed. */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_watchdog_start(0u));

  /* Querying an unstarted watchdog is legal and says it is not running.
   * hal_watchdog_start is never called for real: the IWDG cannot be stopped
   * once armed, so it would reset the part mid-suite. */
  if (!hal_watchdog_is_running()) {
    TEST_ASSERT_EQUAL_UINT32(0u, hal_watchdog_get_timeout_ms());
  } else {
    /* Something armed it before the suite ran; keep it fed. */
    TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_watchdog_kick());
  }
}
#endif /* NAVHAL_CONFIG_DRV_WATCHDOG */

#if NAVHAL_CONFIG_DRV_WWDG
void test_conformance_wwdg_rejects_impossible_window(void) {
  /* A window at or above the timeout can never be satisfied: the counter
   * leaves it only after the reset has fired. Rejected without arming. */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_wwdg_start(10u, 10u));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_wwdg_start(10u, 20u));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_wwdg_start(0u, 0u));

  if (!hal_wwdg_is_running()) {
    (void)hal_wwdg_window_open(); /* must answer, not fault */
  }
}
#endif /* NAVHAL_CONFIG_DRV_WWDG */

#if NAVHAL_CONFIG_DRV_RTC
/* Before hal_rtc_init the driver answers HAL_ERR_NOT_INITIALIZED, and which
 * of the two checks runs first is not part of the contract -- only that a
 * NULL argument never reads as success. */
static bool _conf_rejected(hal_status_t s) {
  return s == HAL_ERR_INVALID_ARG || s == HAL_ERR_NOT_INITIALIZED;
}

void test_conformance_rtc_rejects_null(void) {
  TEST_ASSERT_TRUE(_conf_rejected(hal_rtc_init(NULL)));
  TEST_ASSERT_TRUE(_conf_rejected(hal_rtc_set_datetime(NULL)));
  TEST_ASSERT_TRUE(_conf_rejected(hal_rtc_get_datetime(NULL)));
  TEST_ASSERT_TRUE(
      _conf_rejected(hal_rtc_set_alarm(HAL_RTC_ALARM_A, NULL, NULL)));
  TEST_ASSERT_TRUE(_conf_rejected(hal_rtc_backup_read(0u, NULL)));
}

void test_conformance_rtc_state_queries_are_stable(void) {
  /* Reading twice with nothing in between must agree, whatever the answer. */
  bool set = hal_rtc_is_set();
  TEST_ASSERT_TRUE(set == hal_rtc_is_set());
  bool alarm = hal_rtc_alarm_fired(HAL_RTC_ALARM_A);
  TEST_ASSERT_TRUE(alarm == hal_rtc_alarm_fired(HAL_RTC_ALARM_A));
  bool wake = hal_rtc_wakeup_fired();
  TEST_ASSERT_TRUE(wake == hal_rtc_wakeup_fired());
}
#endif /* NAVHAL_CONFIG_DRV_RTC */

#if NAVHAL_CONFIG_DRV_SPI
void test_conformance_spi_init_hz_rejects_unreachable(void) {
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_spi_init_hz(HAL_SPI_1, &conf_spi_cfg, 0u));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_spi_init_hz(HAL_SPI_1, NULL, 1000000u));
}

void test_conformance_spi_reports_its_clock(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_spi_init(HAL_SPI_1, &conf_spi_cfg));
  /* An initialised instance runs at some rate, and never faster than the bus
   * that feeds it. */
  uint32_t hz = hal_spi_get_clock_hz(HAL_SPI_1);
  TEST_ASSERT_TRUE(hz > 0u);
  TEST_ASSERT_TRUE(hz <= hal_clock_get_sysclk());
}
#endif /* NAVHAL_CONFIG_DRV_SPI */


#if NAVHAL_CONFIG_DRV_TIMEBASE
/* Whether the tick is actually running. Nothing here may call hal_delay_*
 * without checking first: both spin on the tick, so on a target whose
 * application never started a timebase they never return, and the run dies
 * with no summary rather than a failure. */
static bool _conf_timebase_running(void) {
  uint32_t t0 = hal_timebase_get_tick();
  /* The bound only has to outlast one tick period on the slowest core here,
   * and every iteration is a call through the ops table plus an interrupt-safe
   * 32-bit read. On an 8-bit core that is tens of cycles, so a bound chosen
   * for a 84 MHz Cortex-M costs seconds per call on an ATmega -- once per
   * timing case, which is what pushed the AVR HIL run past its timeout. */
  for (uint32_t i = 0u; i < 200000u; ++i) {
    if (hal_timebase_get_tick() != t0) {
      return true;
    }
  }
  return false;
}

void test_conformance_timebase_agrees_with_itself(void) {
  /* The two clocks the whole stack times against are derived from one tick,
   * so they cannot disagree about how long the part has been running. */
  if (!_conf_timebase_running()) {
    /* This target's application never started a timebase; the getters below
     * describe hardware that is not counting. */
    return;
  }
  TEST_ASSERT_TRUE(hal_timebase_get_tick_duration_us() > 0u);
  TEST_ASSERT_TRUE(hal_timebase_get_reload_value() > 0u);

  uint32_t ms = hal_timebase_get_millis();
  uint32_t us = hal_timebase_get_micros();
  /* Sampled in that order, so micros is the later reading; allow a tick of
   * skew plus the millisecond that may have rolled between the two calls. */
  TEST_ASSERT_TRUE(us / 1000u + 1u >= ms);
}

void test_conformance_timebase_advances_monotonically(void) {
  if (!_conf_timebase_running()) {
    return;
  }
  uint32_t t0 = hal_timebase_get_tick();
  uint32_t m0 = hal_timebase_get_millis();

  hal_delay_ms(5u);

  uint32_t elapsed = hal_timebase_get_millis() - m0;
  /* A delay may overshoot; returning early is the failure that matters,
   * with one millisecond of slack for the tick the call started inside. */
  TEST_ASSERT_TRUE(elapsed + 1u >= 5u);
  /* And it must actually come back -- a stalled tick would sit here. */
  TEST_ASSERT_TRUE(elapsed < 1000u);
  TEST_ASSERT_TRUE(hal_timebase_get_tick() != t0);
}

void test_conformance_delay_us_does_not_return_early(void) {
  if (!_conf_timebase_running()) {
    return;
  }
  uint32_t us0 = hal_timebase_get_micros();
  hal_delay_us(2000u);
  uint32_t elapsed = hal_timebase_get_micros() - us0;
  TEST_ASSERT_TRUE(elapsed + hal_timebase_get_tick_duration_us() >= 2000u);
  TEST_ASSERT_TRUE(elapsed < 1000000u);
}

void test_conformance_timebase_rejects_zero_tick(void) {
  /* A zero-microsecond tick is a divide-by-zero waiting to happen in every
   * getter above; it has to be refused before the reload is programmed. */
  uint32_t before = hal_timebase_get_tick_duration_us();
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_timebase_init(0u));
  /* The live timebase the harness is running on must be untouched. */
  TEST_ASSERT_EQUAL_UINT32(before, hal_timebase_get_tick_duration_us());
}
#endif /* NAVHAL_CONFIG_DRV_TIMEBASE */

#if NAVHAL_CONFIG_DRV_TIMER
/* An id past the end of every port's timer enum. The instance-taking entry
 * points are checked against it rather than against a real timer: the board's
 * timers may be driving the harness, and a conformance run must not stop one.
 */
#define CONF_BAD_TIMER ((hal_timer_t)99)

void test_conformance_timer_rejects_unknown_instance(void) {
  static const hal_timer_config_t cfg = {.prescaler = 1u, .auto_reload = 100u};

  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_timer_init(CONF_BAD_TIMER, &cfg));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_timer_init_freq(CONF_BAD_TIMER, 1000u));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_timer_start(CONF_BAD_TIMER));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_timer_stop(CONF_BAD_TIMER));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_timer_reset(CONF_BAD_TIMER));
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_timer_enable_interrupt(CONF_BAD_TIMER));
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_timer_disable_interrupt(CONF_BAD_TIMER));
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_timer_clear_interrupt_flag(CONF_BAD_TIMER));
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_timer_detach_callback(CONF_BAD_TIMER));
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_timer_set_auto_reload(CONF_BAD_TIMER, 100u));
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_timer_set_prescaler(CONF_BAD_TIMER, 1u));
  /* Output compare is optional -- a port that routes it through hal_pwm_*
   * instead answers NOT_SUPPORTED, which is equally not-success. */
  TEST_ASSERT_TRUE(hal_timer_enable_channel(CONF_BAD_TIMER, 1u) != HAL_OK);
  TEST_ASSERT_TRUE(hal_timer_disable_channel(CONF_BAD_TIMER, 1u) != HAL_OK);
  TEST_ASSERT_TRUE(hal_timer_set_compare(CONF_BAD_TIMER, 1u, 10u) != HAL_OK);
}

void test_conformance_timer_getters_are_safe_on_unknown(void) {
  /* The getters have no way to report an error, so an unknown instance has to
   * read as zero rather than dereference past the end of the base table. */
  TEST_ASSERT_EQUAL_UINT32(0u, hal_timer_get_count(CONF_BAD_TIMER));
  TEST_ASSERT_EQUAL_UINT32(0u, hal_timer_get_frequency(CONF_BAD_TIMER));
  TEST_ASSERT_EQUAL_UINT32(0u, hal_timer_get_auto_reload(CONF_BAD_TIMER));
  TEST_ASSERT_EQUAL_UINT32(0u, hal_timer_get_compare(CONF_BAD_TIMER, 1u));
}

void test_conformance_timer_rejects_null_callback(void) {
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_timer_attach_callback(CONF_BAD_TIMER, NULL));
}
#endif /* NAVHAL_CONFIG_DRV_TIMER */

#if NAVHAL_CONFIG_DRV_UART
void test_conformance_uart_rejects_unknown_instance(void) {
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_uart_init((hal_uart_t)99, &conf_uart_cfg));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_uart_init(NAVTEST_UART, NULL));
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_uart_enable_interrupt((hal_uart_t)99, 1u, 0u));
  /* Not initialised, so it has nothing to report and must not claim it does. */
  TEST_ASSERT_TRUE(!hal_uart_available((hal_uart_t)99));
}

void test_conformance_uart_read_until_rejects_null(void) {
  char buf[4];
  TEST_ASSERT_EQUAL_UINT32(
      0u, hal_uart_read_until(NAVTEST_UART, NULL, sizeof buf, '\n'));
  /* No room for even a terminator: nothing may be written through buf. */
  TEST_ASSERT_EQUAL_UINT32(0u,
                           hal_uart_read_until(NAVTEST_UART, buf, 0u, '\n'));
}
#endif /* NAVHAL_CONFIG_DRV_UART */

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
#if NAVHAL_CONFIG_DRV_TIMEBASE
NAVTEST_CASE_DECL(test_conformance_timebase_agrees_with_itself);
#endif
#if NAVHAL_CONFIG_DRV_TIMEBASE
NAVTEST_CASE_DECL(test_conformance_timebase_advances_monotonically);
#endif
#if NAVHAL_CONFIG_DRV_TIMEBASE
NAVTEST_CASE_DECL(test_conformance_delay_us_does_not_return_early);
#endif
#if NAVHAL_CONFIG_DRV_TIMEBASE
NAVTEST_CASE_DECL(test_conformance_timebase_rejects_zero_tick);
#endif
#if NAVHAL_CONFIG_DRV_TIMER
NAVTEST_CASE_DECL(test_conformance_timer_rejects_unknown_instance);
#endif
#if NAVHAL_CONFIG_DRV_TIMER
NAVTEST_CASE_DECL(test_conformance_timer_getters_are_safe_on_unknown);
#endif
#if NAVHAL_CONFIG_DRV_TIMER
NAVTEST_CASE_DECL(test_conformance_timer_rejects_null_callback);
#endif
#if NAVHAL_CONFIG_DRV_UART
NAVTEST_CASE_DECL(test_conformance_uart_rejects_unknown_instance);
#endif
#if NAVHAL_CONFIG_DRV_UART
NAVTEST_CASE_DECL(test_conformance_uart_read_until_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_ETH
NAVTEST_CASE_DECL(test_conformance_eth_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_SDIO
NAVTEST_CASE_DECL(test_conformance_sdio_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_USB_CDC
NAVTEST_CASE_DECL(test_conformance_usb_cdc_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_USB_CDC
NAVTEST_CASE_DECL(test_conformance_usb_cdc_reports_disconnected);
#endif
#if NAVHAL_CONFIG_DRV_WATCHDOG
NAVTEST_CASE_DECL(test_conformance_watchdog_reports_its_limits);
#endif
#if NAVHAL_CONFIG_DRV_WWDG
NAVTEST_CASE_DECL(test_conformance_wwdg_rejects_impossible_window);
#endif
#if NAVHAL_CONFIG_DRV_RTC
NAVTEST_CASE_DECL(test_conformance_rtc_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_RTC
NAVTEST_CASE_DECL(test_conformance_rtc_state_queries_are_stable);
#endif
#if NAVHAL_CONFIG_DRV_SPI
NAVTEST_CASE_DECL(test_conformance_spi_init_hz_rejects_unreachable);
#endif
#if NAVHAL_CONFIG_DRV_SPI
NAVTEST_CASE_DECL(test_conformance_spi_reports_its_clock);
#endif
#if NAVHAL_CONFIG_DRV_CACHE
NAVTEST_CASE_DECL(test_conformance_icache_enable_round_trips);
#endif
#if NAVHAL_CONFIG_DRV_CACHE
NAVTEST_CASE_DECL(test_conformance_dcache_enable_round_trips);
#endif
#if NAVHAL_CONFIG_DRV_CLOCK
NAVTEST_CASE_DECL(test_conformance_clock_getters_are_consistent);
#endif
#if NAVHAL_CONFIG_DRV_CLOCK
NAVTEST_CASE_DECL(test_conformance_clock_rejects_unknown_bus);
#endif
#if NAVHAL_CONFIG_DRV_CRC
NAVTEST_CASE_DECL(test_conformance_crc_reset_restarts_accumulation);
#endif
#if NAVHAL_CONFIG_DRV_DMA
NAVTEST_CASE_DECL(test_conformance_dma_transfer_complete_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_DWT
NAVTEST_CASE_DECL(test_conformance_dwt_counter_advances);
#endif
#if NAVHAL_CONFIG_USE_FPU
NAVTEST_CASE_DECL(test_conformance_fpu_enable_is_idempotent);
#endif
#if NAVHAL_CONFIG_DRV_GPIO
NAVTEST_CASE_DECL(test_conformance_gpio_mode_round_trips);
#endif
#if NAVHAL_CONFIG_DRV_GPIO
NAVTEST_CASE_DECL(test_conformance_gpio_alternate_function_keeps_mode);
#endif
#if NAVHAL_CONFIG_DRV_I2C
NAVTEST_CASE_DECL(test_conformance_i2c_init_status_is_a_bus_mask);
#endif
#if NAVHAL_CONFIG_DRV_MPU
NAVTEST_CASE_DECL(test_conformance_mpu_reports_its_regions);
#endif
#if NAVHAL_CONFIG_DRV_MPU
NAVTEST_CASE_DECL(test_conformance_mpu_rejects_null);
NAVTEST_CASE_DECL(test_conformance_mpu_rejects_misaligned_base);
#endif
#if NAVHAL_CONFIG_DRV_MPU
NAVTEST_CASE_DECL(test_conformance_mpu_enable_round_trips);
#endif
#if NAVHAL_CONFIG_DRV_RESET
NAVTEST_CASE_DECL(test_conformance_reset_reports_a_cause);
#endif
#if NAVHAL_CONFIG_DRV_FLASH
NAVTEST_CASE_DECL(test_conformance_flash_delete_absent_key_is_an_error);
#endif
#if NAVHAL_CONFIG_DRV_ADC
NAVTEST_CASE_DECL(test_conformance_adc_rejects_bad_unit);
#endif
#if NAVHAL_CONFIG_DRV_I2C
NAVTEST_CASE_DECL(test_conformance_i2c_init_rejects_bad_bus);
#endif
#if NAVHAL_CONFIG_DRV_I2C
NAVTEST_CASE_DECL(test_conformance_i2c_deinit_rejects_bad_bus);
#endif
#if NAVHAL_CONFIG_DRV_SPI
NAVTEST_CASE_DECL(test_conformance_spi_init_rejects_bad_instance);
#endif
#if NAVHAL_CONFIG_DRV_TIMER
NAVTEST_CASE_DECL(test_conformance_timer_start_rejects_bad_timer);
#endif
#if NAVHAL_CONFIG_DRV_TIMER
NAVTEST_CASE_DECL(test_conformance_timer_stop_rejects_bad_timer);
#endif
#if NAVHAL_CONFIG_DRV_TIMER
NAVTEST_CASE_DECL(test_conformance_timer_reset_rejects_bad_timer);
#endif
#if NAVHAL_CONFIG_DRV_TIMER
NAVTEST_CASE_DECL(test_conformance_timer_set_divider_rejects_zero);
#endif
#if NAVHAL_CONFIG_DRV_UART
NAVTEST_CASE_DECL(test_conformance_uart_init_rejects_bad_instance);
#endif
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
#if NAVHAL_CONFIG_DRV_TIMEBASE
    NAVTEST_CASE(test_conformance_timebase_agrees_with_itself),
#endif
#if NAVHAL_CONFIG_DRV_TIMEBASE
    NAVTEST_CASE(test_conformance_timebase_advances_monotonically),
#endif
#if NAVHAL_CONFIG_DRV_TIMEBASE
    NAVTEST_CASE(test_conformance_delay_us_does_not_return_early),
#endif
#if NAVHAL_CONFIG_DRV_TIMEBASE
    NAVTEST_CASE(test_conformance_timebase_rejects_zero_tick),
#endif
#if NAVHAL_CONFIG_DRV_TIMER
    NAVTEST_CASE(test_conformance_timer_rejects_unknown_instance),
#endif
#if NAVHAL_CONFIG_DRV_TIMER
    NAVTEST_CASE(test_conformance_timer_getters_are_safe_on_unknown),
#endif
#if NAVHAL_CONFIG_DRV_TIMER
    NAVTEST_CASE(test_conformance_timer_rejects_null_callback),
#endif
#if NAVHAL_CONFIG_DRV_UART
    NAVTEST_CASE(test_conformance_uart_rejects_unknown_instance),
#endif
#if NAVHAL_CONFIG_DRV_UART
    NAVTEST_CASE(test_conformance_uart_read_until_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_ETH
    NAVTEST_CASE(test_conformance_eth_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_SDIO
    NAVTEST_CASE(test_conformance_sdio_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_USB_CDC
    NAVTEST_CASE(test_conformance_usb_cdc_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_USB_CDC
    NAVTEST_CASE(test_conformance_usb_cdc_reports_disconnected),
#endif
#if NAVHAL_CONFIG_DRV_WATCHDOG
    NAVTEST_CASE(test_conformance_watchdog_reports_its_limits),
#endif
#if NAVHAL_CONFIG_DRV_WWDG
    NAVTEST_CASE(test_conformance_wwdg_rejects_impossible_window),
#endif
#if NAVHAL_CONFIG_DRV_RTC
    NAVTEST_CASE(test_conformance_rtc_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_RTC
    NAVTEST_CASE(test_conformance_rtc_state_queries_are_stable),
#endif
#if NAVHAL_CONFIG_DRV_SPI
    NAVTEST_CASE(test_conformance_spi_init_hz_rejects_unreachable),
#endif
#if NAVHAL_CONFIG_DRV_SPI
    NAVTEST_CASE(test_conformance_spi_reports_its_clock),
#endif
#if NAVHAL_CONFIG_DRV_CACHE
    NAVTEST_CASE(test_conformance_icache_enable_round_trips),
#endif
#if NAVHAL_CONFIG_DRV_CACHE
    NAVTEST_CASE(test_conformance_dcache_enable_round_trips),
#endif
#if NAVHAL_CONFIG_DRV_CLOCK
    NAVTEST_CASE(test_conformance_clock_getters_are_consistent),
#endif
#if NAVHAL_CONFIG_DRV_CLOCK
    NAVTEST_CASE(test_conformance_clock_rejects_unknown_bus),
#endif
#if NAVHAL_CONFIG_DRV_CRC
    NAVTEST_CASE(test_conformance_crc_reset_restarts_accumulation),
#endif
#if NAVHAL_CONFIG_DRV_DMA
    NAVTEST_CASE(test_conformance_dma_transfer_complete_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_DWT
    NAVTEST_CASE(test_conformance_dwt_counter_advances),
#endif
#if NAVHAL_CONFIG_USE_FPU
    NAVTEST_CASE(test_conformance_fpu_enable_is_idempotent),
#endif
#if NAVHAL_CONFIG_DRV_GPIO
    NAVTEST_CASE(test_conformance_gpio_mode_round_trips),
#endif
#if NAVHAL_CONFIG_DRV_GPIO
    NAVTEST_CASE(test_conformance_gpio_alternate_function_keeps_mode),
#endif
#if NAVHAL_CONFIG_DRV_I2C
    NAVTEST_CASE(test_conformance_i2c_init_status_is_a_bus_mask),
#endif
#if NAVHAL_CONFIG_DRV_MPU
    NAVTEST_CASE(test_conformance_mpu_reports_its_regions),
#endif
#if NAVHAL_CONFIG_DRV_MPU
    NAVTEST_CASE(test_conformance_mpu_rejects_null),
    NAVTEST_CASE(test_conformance_mpu_rejects_misaligned_base),
#endif
#if NAVHAL_CONFIG_DRV_MPU
    NAVTEST_CASE(test_conformance_mpu_enable_round_trips),
#endif
#if NAVHAL_CONFIG_DRV_RESET
    NAVTEST_CASE(test_conformance_reset_reports_a_cause),
#endif
#if NAVHAL_CONFIG_DRV_FLASH
    NAVTEST_CASE(test_conformance_flash_delete_absent_key_is_an_error),
#endif
#if NAVHAL_CONFIG_DRV_ADC
    NAVTEST_CASE(test_conformance_adc_rejects_bad_unit),
#endif
#if NAVHAL_CONFIG_DRV_I2C
    NAVTEST_CASE(test_conformance_i2c_init_rejects_bad_bus),
#endif
#if NAVHAL_CONFIG_DRV_I2C
    NAVTEST_CASE(test_conformance_i2c_deinit_rejects_bad_bus),
#endif
#if NAVHAL_CONFIG_DRV_SPI
    NAVTEST_CASE(test_conformance_spi_init_rejects_bad_instance),
#endif
#if NAVHAL_CONFIG_DRV_TIMER
    NAVTEST_CASE(test_conformance_timer_start_rejects_bad_timer),
#endif
#if NAVHAL_CONFIG_DRV_TIMER
    NAVTEST_CASE(test_conformance_timer_stop_rejects_bad_timer),
#endif
#if NAVHAL_CONFIG_DRV_TIMER
    NAVTEST_CASE(test_conformance_timer_reset_rejects_bad_timer),
#endif
#if NAVHAL_CONFIG_DRV_TIMER
    NAVTEST_CASE(test_conformance_timer_set_divider_rejects_zero),
#endif
#if NAVHAL_CONFIG_DRV_UART
    NAVTEST_CASE(test_conformance_uart_init_rejects_bad_instance),
#endif
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
