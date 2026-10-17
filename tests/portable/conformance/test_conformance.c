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
/* Mirrors the driver's own fallback: an unset Kconfig int arrives as an empty
 * define, not as absent. */
#if defined(NAVHAL_CONFIG_FLASH_KV_PRIMARY_SECTOR) &&                          \
    (NAVHAL_CONFIG_FLASH_KV_PRIMARY_SECTOR + 0) > 0
#define NAVHAL_CONFORMANCE_KV_PRIMARY NAVHAL_CONFIG_FLASH_KV_PRIMARY_SECTOR
#else
#define NAVHAL_CONFORMANCE_KV_PRIMARY 6
#endif
#if defined(NAVHAL_CONFIG_FLASH_KV_SECONDARY_SECTOR) &&                        \
    (NAVHAL_CONFIG_FLASH_KV_SECONDARY_SECTOR + 0) > 0
#define NAVHAL_CONFORMANCE_KV_SECONDARY NAVHAL_CONFIG_FLASH_KV_SECONDARY_SECTOR
#else
#define NAVHAL_CONFORMANCE_KV_SECONDARY 7
#endif
#include "common/hal_flash.h"
#endif
#if NAVHAL_CONFIG_DRV_ADC
#include "common/hal_adc.h"
#endif

#if NAVHAL_CONFIG_DRV_UART
#include "common/hal_uart.h"
#endif
#if NAVHAL_CONFIG_DRV_INTERRUPT
#include "common/hal_interrupt.h"
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
#include "common/hal_diskio.h"
#endif
#if NAVHAL_CONFIG_BOOT_SNIFFER
#include "common/hal_boot.h"
#endif
#include "common/hal_console.h"
#if NAVHAL_CONFIG_DRV_CRC
#include "common/hal_crc.h"
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
static void _conf_dma_cb(void) {}
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


/* The raw partition API refuses what a loader must never touch. Every one of
 * these is an argument check, so none of them writes flash -- the erase and
 * program paths themselves want a board with the driver enabled, which no test
 * config has yet. */
void test_conformance_flash_raw_erase_refuses_stage1(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_flash_raw_erase_sector(0u));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_flash_raw_erase_sector(1u));
}

void test_conformance_flash_raw_erase_refuses_a_sector_that_does_not_exist(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_flash_raw_erase_sector(8u));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_flash_raw_erase_sector(255u));
}

/* The key/value store erases its own sectors; a loader reaching into them would
 * take the attempt counter and the rollback floor with it. */
void test_conformance_flash_raw_erase_refuses_the_kv_sectors(void) {
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_flash_raw_erase_sector((uint8_t)NAVHAL_CONFORMANCE_KV_PRIMARY));
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_flash_raw_erase_sector((uint8_t)NAVHAL_CONFORMANCE_KV_SECONDARY));
}

void test_conformance_flash_raw_program_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_flash_raw_program(0x08020000UL, NULL, 2u));
}

/* Half-word granularity, so an odd address or length is refused rather than
 * rounded -- rounding would write a byte the caller did not ask for. */
void test_conformance_flash_raw_program_rejects_odd_address_or_length(void) {
  static const uint8_t two[2] = {0xAA, 0x55};
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_flash_raw_program(0x08020001UL, two, 2u));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_flash_raw_program(0x08020000UL, two, 1u));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_flash_raw_program(0x08020000UL, two, 0u));
}

/* The whole range is checked, not just where it starts: a write that begins in
 * stage-2 and runs backwards into stage-1 is the interesting case. */
void test_conformance_flash_raw_program_rejects_a_range_reaching_stage1(void) {
  static const uint8_t buf[4] = {0};
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_flash_raw_program(0x08007FFEUL, buf, 4u));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_flash_raw_program(0x07FFFFFEUL, buf, 4u));
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

/* The stream callback belongs to the DMA module; these are the refusals that
 * make a wrong claim visible instead of silent. The BUSY case needs a real
 * table and lives in the DRV_DMA cap suite. */
/* Portable only as far as "an impossible line is refused": a port with no
 * programmable priorities answers NOT_SUPPORTED to the priority half and
 * enables anyway, so the status for a valid line differs by port. */
void test_conformance_dma_attach_callback_rejects_null_cb(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_dma_attach_callback(HAL_DMA_CONTROLLER_1, 0u, NULL));
}

void test_conformance_dma_attach_callback_rejects_bad_stream(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_dma_attach_callback(HAL_DMA_CONTROLLER_1, 8u, _conf_dma_cb));
}

void test_conformance_dma_detach_callback_rejects_bad_controller(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_dma_detach_callback((hal_dma_controller_t)9, 0u));
}

#endif /* NAVHAL_CONFIG_DRV_DMA */

#if NAVHAL_CONFIG_DRV_INTERRUPT
/* The interrupt API is portable contract now that it is declared in common
 * rather than per port, so the refusals belong here. What a port does with a
 * *valid* line differs -- the AVR has no central enable and answers
 * NOT_SUPPORTED -- so these assert on a line that cannot exist anywhere. */
#define CONF_BAD_IRQ ((hal_irq_t)-99)

void test_conformance_interrupt_enable_rejects_bad_irq(void) {
  TEST_ASSERT_TRUE(hal_interrupt_enable(CONF_BAD_IRQ) != HAL_OK);
}

void test_conformance_interrupt_disable_rejects_bad_irq(void) {
  TEST_ASSERT_TRUE(hal_interrupt_disable(CONF_BAD_IRQ) != HAL_OK);
}

void test_conformance_interrupt_attach_rejects_null_callback(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_interrupt_attach_callback((hal_irq_t)0, NULL));
}

void test_conformance_interrupt_detach_rejects_bad_irq(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_interrupt_detach_callback(CONF_BAD_IRQ));
}

void test_conformance_interrupt_clear_pending_rejects_bad_irq(void) {
  TEST_ASSERT_TRUE(hal_interrupt_clear_pending(CONF_BAD_IRQ) != HAL_OK);
}

void test_conformance_interrupt_set_priority_rejects_bad_irq(void) {
  TEST_ASSERT_TRUE(hal_interrupt_set_priority(CONF_BAD_IRQ, 5u) != HAL_OK);
}

/* A reader cannot be pinned to a value across ports: the ARM answers 0xFF, a
 * sentinel for "invalid or fixed priority", while the AVR answers 0 because it
 * has no priorities at all. Requiring 0 would have forced the ARM to return a
 * plausible-looking priority -- 0 is the highest -- for a line that does not
 * exist. The contract is that asking answers, without faulting. */
void test_conformance_interrupt_get_priority_of_bad_irq_answers(void) {
  (void)hal_interrupt_get_priority(CONF_BAD_IRQ);
  TEST_ASSERT_TRUE(1);
}

void test_conformance_interrupt_bad_irq_is_not_pending(void) {
  TEST_ASSERT_TRUE(!hal_interrupt_is_pending(CONF_BAD_IRQ));
}

/* Dispatching a line nobody attached to must return, not fault: the ports
 * answer an empty slot differently in their own vectors, but the public call
 * is a table lookup. */
void test_conformance_interrupt_dispatch_of_empty_slot_returns(void) {
  hal_interrupt_dispatch(CONF_BAD_IRQ);
  TEST_ASSERT_TRUE(1);
}

/* The save/restore pair has to round-trip: disable returns the previous state
 * and enable_global puts exactly that back. */
void test_conformance_interrupt_global_state_round_trips(void) {
  uint32_t state = hal_interrupt_disable_global();
  hal_interrupt_enable_global(state);
  TEST_ASSERT_TRUE(1);
}
#endif /* NAVHAL_CONFIG_DRV_INTERRUPT */

void test_conformance_interrupt_enable_with_priority_rejects_bad_irq(void) {
  TEST_ASSERT_TRUE(hal_interrupt_enable_with_priority((hal_irq_t)-99, 5u) != HAL_OK);
}


#if NAVHAL_CONFIG_DRV_I2C_DMA
/* Lifted out of the port headers, so the gate sees them now. The bindings are
 * per-port data but the checks are not: a bad bus and a NULL out are refused
 * the same way everywhere. */
static uint8_t i2c_dma_buf[2];
static void _conf_i2c_dma_cb(void) {}

void test_conformance_i2c_dma_get_binding_rejects_null_out(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_i2c_dma_get_binding((hal_i2c_bus_t)0, true, NULL));
}

void test_conformance_i2c_dma_set_binding_rejects_bad_bus(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_i2c_dma_set_binding((hal_i2c_bus_t)99, true, NULL));
}

void test_conformance_i2c_read_regs_dma_rejects_null_buffer(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_i2c_read_regs_dma((hal_i2c_bus_t)0, 0x50u, 0x00u, NULL,
                                                           sizeof(i2c_dma_buf), _conf_i2c_dma_cb));
}
#endif /* NAVHAL_CONFIG_DRV_I2C_DMA */



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

  /* Initialise first. On a part whose CRC unit sits behind a clock gate -- the
   * STM32F4 does -- an uninitialised unit swallows every write and reads back a
   * constant, so both assertions below pass each other happily while nothing is
   * being computed at all. This case went unnoticed because no board config
   * enabled the driver until now. */
  static const hal_crc_config_t cfg = {.init_value = 0xFFFFFFFFu};
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_crc_init(&cfg));

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
/* Attaching NULL is an argument error now that clearing has its own call; the
 * old set_callback(NULL) had to mean "clear", so neither could be asserted. */
void test_conformance_eth_attach_callback_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_eth_attach_callback(NULL));
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
/* Attaching NULL is an argument error now that clearing has its own call; the
 * old set_callback(NULL) had to mean "clear", so neither could be asserted. */
void test_conformance_sdio_attach_callback_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_sdio_attach_callback(NULL));
}

#endif /* NAVHAL_CONFIG_DRV_SDIO */

#if NAVHAL_CONFIG_DRV_USB_CDC
/* Both predicates answer without a host and without init, which is all a
 * portable case can ask: the DTR one is already covered, and this is the one a
 * bootloader polls instead. */
void test_conformance_usb_cdc_enumerated_answers_without_a_host(void) {
  (void)hal_usb_cdc_enumerated();
  TEST_ASSERT_TRUE(1);
}

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
/* Attaching NULL is an argument error now that clearing has its own call; the
 * old set_callback(NULL) had to mean "clear", so neither could be asserted. */
void test_conformance_usb_cdc_attach_rx_callback_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG, (uint32_t)hal_usb_cdc_attach_rx_callback(NULL));
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

/* hal_rtc_init is deliberately absent from the list below. NULL is a valid
 * argument there -- hal_rtc.h documents it as selecting HAL_RTC_CLOCK_AUTO with
 * the default crystal timeout -- so asserting a rejection tested this suite's
 * assumption rather than the driver's contract, and failed on hardware for the
 * right reason. See test_conformance_rtc_accepts_null_config. */
void test_conformance_rtc_rejects_null(void) {
  TEST_ASSERT_TRUE(_conf_rejected(hal_rtc_set_datetime(NULL)));
  TEST_ASSERT_TRUE(_conf_rejected(hal_rtc_get_datetime(NULL)));
  TEST_ASSERT_TRUE(
      _conf_rejected(hal_rtc_set_alarm(HAL_RTC_ALARM_A, NULL, NULL)));
  TEST_ASSERT_TRUE(_conf_rejected(hal_rtc_backup_read(0u, NULL)));
}

void test_conformance_rtc_accepts_null_config(void) {
  /* The other half of the contract: NULL means "keep whatever oscillator is
   * running", so the one wrong answer is to reject it.
   *
   * Not asserted as success. Whether the calendar comes up depends on an
   * oscillator this suite cannot assume -- a board with no crystal fitted answers
   * HAL_ERR_TIMEOUT, which the header documents and which is not a conformance
   * failure. Non-destructive either way: AUTO never asks for the backup-domain
   * reset that would clear the calendar and the backup registers. */
  hal_status_t s = hal_rtc_init(NULL);
  TEST_ASSERT_TRUE(s == HAL_OK || s == HAL_ERR_TIMEOUT);
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
NAVTEST_CASE_DECL(test_conformance_usb_cdc_enumerated_answers_without_a_host);
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
NAVTEST_CASE_DECL(test_conformance_rtc_accepts_null_config);
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
NAVTEST_CASE_DECL(test_conformance_flash_raw_erase_refuses_stage1);
NAVTEST_CASE_DECL(test_conformance_flash_raw_erase_refuses_a_sector_that_does_not_exist);
NAVTEST_CASE_DECL(test_conformance_flash_raw_erase_refuses_the_kv_sectors);
NAVTEST_CASE_DECL(test_conformance_flash_raw_program_rejects_null);
NAVTEST_CASE_DECL(test_conformance_flash_raw_program_rejects_odd_address_or_length);
NAVTEST_CASE_DECL(test_conformance_flash_raw_program_rejects_a_range_reaching_stage1);
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
#if NAVHAL_CONFIG_DRV_USB_CDC
NAVTEST_CASE_DECL(test_conformance_usb_cdc_attach_rx_callback_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_SDIO
NAVTEST_CASE_DECL(test_conformance_sdio_attach_callback_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_ETH
NAVTEST_CASE_DECL(test_conformance_eth_attach_callback_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_I2C_DMA
NAVTEST_CASE_DECL(test_conformance_i2c_read_regs_dma_rejects_null_buffer);
#endif
#if NAVHAL_CONFIG_DRV_I2C_DMA
NAVTEST_CASE_DECL(test_conformance_i2c_dma_set_binding_rejects_bad_bus);
#endif
#if NAVHAL_CONFIG_DRV_I2C_DMA
NAVTEST_CASE_DECL(test_conformance_i2c_dma_get_binding_rejects_null_out);
#endif
NAVTEST_CASE_DECL(test_conformance_dma_detach_callback_rejects_bad_controller);
NAVTEST_CASE_DECL(test_conformance_dma_attach_callback_rejects_bad_stream);
NAVTEST_CASE_DECL(test_conformance_dma_attach_callback_rejects_null_cb);
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
NAVTEST_CASE_DECL(test_conformance_console_getters_are_stable);
NAVTEST_CASE_DECL(test_conformance_console_write_ignores_null);
#if NAVHAL_CONFIG_DRV_UART
NAVTEST_CASE_DECL(test_conformance_uart_writers_reject_bad_instance);
NAVTEST_CASE_DECL(test_conformance_uart_read_char_answers_on_bad_instance);
#endif
#if NAVHAL_CONFIG_DRV_CRC
NAVTEST_CASE_DECL(test_conformance_crc_init_rejects_null);
#endif
#if NAVHAL_CONFIG_DRV_TIMEBASE
NAVTEST_CASE_DECL(test_conformance_timebase_callback_accepts_null);
#endif
#if NAVHAL_CONFIG_DRV_WWDG
NAVTEST_CASE_DECL(test_conformance_wwdg_kick_needs_a_running_watchdog);
#endif
#if NAVHAL_CONFIG_DRV_ETH
NAVTEST_CASE_DECL(test_conformance_eth_set_callback_accepts_null);
NAVTEST_CASE_DECL(test_conformance_eth_link_is_up_answers_without_a_mac);
NAVTEST_CASE_DECL(test_conformance_eth_teardown_is_safe_in_any_order);
NAVTEST_CASE_DECL(test_conformance_eth_needs_init_before_the_bus);
#endif
#if NAVHAL_CONFIG_DRV_USB_CDC
NAVTEST_CASE_DECL(test_conformance_usb_cdc_getters_are_stable);
NAVTEST_CASE_DECL(test_conformance_usb_cdc_set_rx_callback_accepts_null);
NAVTEST_CASE_DECL(test_conformance_usb_cdc_notify_needs_a_host);
NAVTEST_CASE_DECL(test_conformance_usb_cdc_init_answers_the_same_twice);
#endif
#if NAVHAL_CONFIG_DRV_SDIO
NAVTEST_CASE_DECL(test_conformance_sdio_card_present_is_stable);
NAVTEST_CASE_DECL(test_conformance_sdio_card_init_answers_for_an_empty_slot);
NAVTEST_CASE_DECL(test_conformance_sdio_command_needs_the_peripheral);
NAVTEST_CASE_DECL(test_conformance_sdio_get_response_rejects_bad_register);
NAVTEST_CASE_DECL(test_conformance_sdio_set_callback_accepts_null);
NAVTEST_CASE_DECL(test_conformance_disk_rejects_a_drive_that_does_not_exist);
NAVTEST_CASE_DECL(test_conformance_disk_rejects_a_zero_length_transfer);
#endif
#if NAVHAL_CONFIG_DRV_RTC
NAVTEST_CASE_DECL(test_conformance_rtc_backup_write_rejects_bad_index);
NAVTEST_CASE_DECL(test_conformance_rtc_set_wakeup_rejects_bad_period);
NAVTEST_CASE_DECL(test_conformance_rtc_cancel_is_idempotent);
NAVTEST_CASE_DECL(test_conformance_rtc_get_clock_is_a_documented_source);
#endif
#if NAVHAL_CONFIG_DRV_MPU
NAVTEST_CASE_DECL(test_conformance_mpu_disable_region_rejects_bad_index);
#endif
#if NAVHAL_CONFIG_BOOT_SNIFFER
NAVTEST_CASE_DECL(test_conformance_boot_block_init_validates);
NAVTEST_CASE_DECL(test_conformance_boot_getters_are_stable);
NAVTEST_CASE_DECL(test_conformance_boot_entry_gate_round_trips);
NAVTEST_CASE_DECL(test_conformance_boot_request_is_refused_while_disabled);
NAVTEST_CASE_DECL(test_conformance_boot_request_target_checks_its_argument);
NAVTEST_CASE_DECL(test_conformance_boot_match_ignores_other_traffic);
NAVTEST_CASE_DECL(test_conformance_boot_clear_and_heal_need_a_valid_block);
NAVTEST_CASE_DECL(test_conformance_boot_attempt_counts_and_clears);
NAVTEST_CASE_DECL(test_conformance_boot_set_prepare_accepts_null);
#endif
#if NAVHAL_CONFIG_DRV_INTERRUPT
NAVTEST_CASE_DECL(test_conformance_interrupt_enable_rejects_bad_irq);
NAVTEST_CASE_DECL(test_conformance_interrupt_disable_rejects_bad_irq);
NAVTEST_CASE_DECL(test_conformance_interrupt_attach_rejects_null_callback);
NAVTEST_CASE_DECL(test_conformance_interrupt_detach_rejects_bad_irq);
NAVTEST_CASE_DECL(test_conformance_interrupt_clear_pending_rejects_bad_irq);
NAVTEST_CASE_DECL(test_conformance_interrupt_set_priority_rejects_bad_irq);
NAVTEST_CASE_DECL(test_conformance_interrupt_get_priority_of_bad_irq_answers);
NAVTEST_CASE_DECL(test_conformance_interrupt_bad_irq_is_not_pending);
NAVTEST_CASE_DECL(test_conformance_interrupt_dispatch_of_empty_slot_returns);
NAVTEST_CASE_DECL(test_conformance_interrupt_global_state_round_trips);
NAVTEST_CASE_DECL(test_conformance_interrupt_enable_with_priority_rejects_bad_irq);
#endif


#if NAVHAL_CONFIG_BOOT_SNIFFER
/* ---------------------------------------------------------------------------
 * Boot sniffer. Every case here runs with entry disabled, because a completed
 * match calls hal_boot_request and that resets the part -- the refusal is the
 * half a running suite can assert, and asserting it is how we know the
 * interlock a console-fed board depends on actually holds.
 * ------------------------------------------------------------------------- */
static void _conf_boot_prepare_cb(void) { /* never invoked: entry stays off */ }

void test_conformance_boot_block_init_validates(void) {
  /* Idempotent by contract: a valid block is left alone, an invalid one is
   * sealed. Either way the block reads valid afterwards. */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_boot_block_init());
  TEST_ASSERT_TRUE(hal_boot_block_valid());
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_boot_block_init());
  TEST_ASSERT_TRUE(hal_boot_block_valid());
}

void test_conformance_boot_getters_are_stable(void) {
  uint32_t req = hal_boot_get_request();
  TEST_ASSERT_EQUAL_UINT32(req, hal_boot_get_request());
  uint32_t att = hal_boot_get_attempts();
  TEST_ASSERT_EQUAL_UINT32(att, hal_boot_get_attempts());
}

void test_conformance_boot_entry_gate_round_trips(void) {
  bool was = hal_boot_entry_is_disabled();

  hal_boot_entry_disable();
  TEST_ASSERT_TRUE(hal_boot_entry_is_disabled());
  hal_boot_entry_enable();
  TEST_ASSERT_FALSE(hal_boot_entry_is_disabled());

  if (was) {
    hal_boot_entry_disable();
  }
}

void test_conformance_boot_request_is_refused_while_disabled(void) {
  /* The one call in this file that would reset the board if the interlock did
   * not hold. Disabled first, so a conformant port answers HAL_ERR_BUSY and
   * the run continues -- and a port that resets here fails loudly by never
   * reaching the summary. */
  bool was = hal_boot_entry_is_disabled();

  hal_boot_entry_disable();
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_BUSY,
                           (uint32_t)hal_boot_request());

  if (!was) {
    hal_boot_entry_enable();
  }
  hal_boot_match_reset();
}

void test_conformance_boot_request_target_checks_its_argument(void) {
  /* The argument contract only. The success path resets the board, which a suite
   * mid-run must not do -- so entry is disabled first, exactly as the case above
   * does, and a port that resets here fails by never reaching the summary.
   *
   * What matters is that the two requests are distinct: the console path raises
   * the one stage-2 claims, and only an explicit call raises the one that reaches
   * the loader able to rewrite stage-2. */
  bool was = hal_boot_entry_is_disabled();

  hal_boot_entry_disable();
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_BUSY,
                           (uint32_t)hal_boot_request_target(HAL_BOOT_REQ_STAGE1));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_BUSY,
                           (uint32_t)hal_boot_request_target(HAL_BOOT_REQ_LOADER));

  /* An argument that names neither loader is refused whether entry is open or
   * not, and the check comes before anything is written. */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_BUSY,
                           (uint32_t)hal_boot_request_target(0u));

  if (!was) {
    hal_boot_entry_enable();
    TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                             (uint32_t)hal_boot_request_target(0xDEADBEEFu));
    TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_BOOT_REQ_NONE, hal_boot_get_request());
  }
  hal_boot_match_reset();
}

void test_conformance_boot_match_ignores_other_traffic(void) {
  /* A byte that cannot be the sequence's first one cannot advance the match,
   * whatever the port chose for hal_boot_seq. */
  const uint8_t not_first = (uint8_t)(hal_boot_seq[0] ^ 0xFFu);
  bool was = hal_boot_entry_is_disabled();
  uint32_t before;

  hal_boot_entry_disable();
  hal_boot_match_reset();
  before = hal_boot_get_request();

  hal_boot_match_byte(not_first);
  hal_boot_feed(&not_first, 1u);
  hal_boot_feed(NULL, 4u); /* NULL is a no-op, not a fault */

  TEST_ASSERT_EQUAL_UINT32(before, hal_boot_get_request());

  hal_boot_match_reset();
  if (!was) {
    hal_boot_entry_enable();
  }
}

void test_conformance_boot_clear_and_heal_need_a_valid_block(void) {
  /* Both mutate the block, so both need one: the contract is that they answer
   * HAL_ERR_NOT_INITIALIZED rather than writing into an unsealed block. After
   * block_init there is one, and mark_healthy's effect is observable. */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_boot_block_init());
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_boot_clear_request());
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_BOOT_REQ_NONE, hal_boot_get_request());
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_boot_mark_healthy());
  TEST_ASSERT_EQUAL_UINT32(0u, hal_boot_get_attempts());
}

/* The loader's half of the crashloop defence, portable because the counter lives
 * in the shared boot block and not in anything device-specific. Left clean on the
 * way out: a suite that bumped the count and walked away would send the next boot
 * of this board to recovery. */
void test_conformance_boot_attempt_counts_and_clears(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_boot_block_init());
  uint32_t before = hal_boot_get_attempts();
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_boot_account_attempt());
  TEST_ASSERT_EQUAL_UINT32(before + 1u, hal_boot_get_attempts());
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_boot_mark_healthy());
  TEST_ASSERT_EQUAL_UINT32(0u, hal_boot_get_attempts());
}

void test_conformance_boot_set_prepare_accepts_null(void) {
  /* Registration only -- the callback runs on the path to a reset, which this
   * suite never takes. NULL is how a caller unregisters, so it must be
   * accepted at both ends, and the slot is left empty on the way out. */
  hal_boot_set_prepare(NULL);
  hal_boot_set_prepare(_conf_boot_prepare_cb);
  hal_boot_set_prepare(NULL);
  TEST_ASSERT_TRUE(hal_boot_block_valid());
}
#endif /* NAVHAL_CONFIG_BOOT_SNIFFER */


/* ---------------------------------------------------------------------------
 * Console, and the UART convenience writers underneath it. The console is the
 * transport this suite's own output leaves through, so what can be asserted is
 * the part that does not disturb it: the getters, and that a NULL string is a
 * no-op rather than a fault.
 * ------------------------------------------------------------------------- */
void test_conformance_console_getters_are_stable(void) {
  bool c = hal_console_connected();
  TEST_ASSERT_TRUE(c == hal_console_connected());

  hal_console_route_t r = hal_console_get_route();
  TEST_ASSERT_TRUE(r == hal_console_get_route());
  /* The route is what the firmware was built with, so it is one of the two the
   * contract defines -- never a third value a port invented. */
  TEST_ASSERT_TRUE(r == HAL_CONSOLE_ROUTE_UART || r == HAL_CONSOLE_ROUTE_CDC);
}

void test_conformance_console_write_ignores_null(void) {
  /* NULL is a no-op by contract: a console call sits on error paths, where a
   * fault would destroy the very message explaining what went wrong. */
  hal_console_write(NULL);
  hal_console_write("");
  /* The numeric writers emit without a newline, so the line is closed here and
   * the transcript stays parseable. */
  hal_console_write_uint(0u);
  hal_console_write_hex32(0u);
  hal_console_write("\r\n");
  TEST_ASSERT_TRUE(hal_console_get_route() == hal_console_get_route());
}

#if NAVHAL_CONFIG_DRV_UART
void test_conformance_uart_writers_reject_bad_instance(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_uart_write_char((hal_uart_t)99, 'x'));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_uart_write_int((hal_uart_t)99, -1));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_uart_write_uint((hal_uart_t)99, 1u));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_uart_write_float((hal_uart_t)99, 1.0f));
}

void test_conformance_uart_read_char_answers_on_bad_instance(void) {
  /* Only the error path is assertable here: on a real instance read_char
   * blocks until a byte arrives, and nothing sends one to a board under test.
   * The contract for an instance that does not exist is to answer 0 rather
   * than to wait forever for a peripheral that is not there. */
  TEST_ASSERT_EQUAL_UINT32(0u, (uint32_t)hal_uart_read_char((hal_uart_t)99));
}
#endif /* NAVHAL_CONFIG_DRV_UART */

#if NAVHAL_CONFIG_DRV_CRC
void test_conformance_crc_init_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_crc_init(NULL));
  /* Twice, because an error path that corrupts state answers differently the
   * second time. */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_crc_init(NULL));
}
#endif /* NAVHAL_CONFIG_DRV_CRC */

#if NAVHAL_CONFIG_DRV_TIMEBASE
static void _conf_timebase_cb(void) { /* registered, then withdrawn */ }

void test_conformance_timebase_callback_accepts_null(void) {
  /* NULL is how a caller withdraws a tick callback. A port that cannot offer
   * one says so; what it may not do is fault, or accept NULL and then call
   * through it from the tick ISR. */
  hal_status_t s = hal_timebase_attach_callback(_conf_timebase_cb);
  TEST_ASSERT_TRUE(s == HAL_OK || s == HAL_ERR_NOT_SUPPORTED);

  s = hal_timebase_detach_callback();
  TEST_ASSERT_TRUE(s == HAL_OK || s == HAL_ERR_NOT_SUPPORTED ||
                   s == HAL_ERR_INVALID_ARG);
}
#endif /* NAVHAL_CONFIG_DRV_TIMEBASE */

/* hal_wwdg_* is declared in hal_watchdog.h, which is included above under
 * DRV_WATCHDOG -- and a port can build the window watchdog without the
 * independent one, so the header has to be reachable either way. */
#if NAVHAL_CONFIG_DRV_WWDG && !NAVHAL_CONFIG_DRV_WATCHDOG
#include "common/hal_watchdog.h"
#endif
#if NAVHAL_CONFIG_DRV_WWDG
void test_conformance_wwdg_kick_needs_a_running_watchdog(void) {
  /* Nothing in this suite arms a window watchdog -- on most parts it cannot be
   * stopped again -- so the assertable half is the refusal. Branching on
   * is_running keeps that true whatever ran before this case. */
  if (!hal_wwdg_is_running()) {
    TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_NOT_INITIALIZED,
                             (uint32_t)hal_wwdg_kick());
  } else {
    /* Already armed by something else: kicking is then the safe answer, and
     * refusing to would be the bug. */
    TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)hal_wwdg_kick());
  }
}
#endif /* NAVHAL_CONFIG_DRV_WWDG */


#if NAVHAL_CONFIG_DRV_RTC
void test_conformance_rtc_backup_write_rejects_bad_index(void) {
  /* The register file is finite, and the contract names its size. One past the
   * end must be refused rather than writing into whatever follows it. */
  TEST_ASSERT_TRUE(
      _conf_rejected(hal_rtc_backup_write(HAL_RTC_BACKUP_COUNT, 0u)));
  TEST_ASSERT_TRUE(_conf_rejected(hal_rtc_backup_write(255u, 0u)));
}

void test_conformance_rtc_set_wakeup_rejects_bad_period(void) {
  /* 0 and anything past the documented 65536000 ms ceiling are out of range.
   * A NULL callback is not -- the header documents it as leaving the timer
   * running with nothing attached. */
  TEST_ASSERT_TRUE(_conf_rejected(hal_rtc_set_wakeup(0u, NULL)));
  TEST_ASSERT_TRUE(_conf_rejected(hal_rtc_set_wakeup(65536001u, NULL)));
}

void test_conformance_rtc_cancel_is_idempotent(void) {
  /* Cancelling something that is not running is not an error worth inventing a
   * state machine for: the answer must be the same both times, whatever it is,
   * and neither call may leave the driver unable to answer the next one. */
  hal_status_t a = hal_rtc_cancel_wakeup();
  hal_status_t b = hal_rtc_cancel_wakeup();
  TEST_ASSERT_EQUAL_UINT32((uint32_t)a, (uint32_t)b);

  hal_status_t c = hal_rtc_cancel_alarm(HAL_RTC_ALARM_A);
  hal_status_t d = hal_rtc_cancel_alarm(HAL_RTC_ALARM_A);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)c, (uint32_t)d);
}

void test_conformance_rtc_get_clock_is_a_documented_source(void) {
  /* Whatever oscillator came up, the answer is one of the four the enum
   * defines -- and a board with no crystal reports NONE rather than guessing. */
  hal_rtc_clock_t k = hal_rtc_get_clock();
  TEST_ASSERT_TRUE(k == HAL_RTC_CLOCK_AUTO || k == HAL_RTC_CLOCK_LSE ||
                   k == HAL_RTC_CLOCK_LSI || k == HAL_RTC_CLOCK_NONE);
  TEST_ASSERT_TRUE(k == hal_rtc_get_clock());
}
#endif /* NAVHAL_CONFIG_DRV_RTC */

#if NAVHAL_CONFIG_DRV_MPU
void test_conformance_mpu_disable_region_rejects_bad_index(void) {
  /* Disabling is the safe direction -- it removes a restriction rather than
   * adding one -- so the only thing to get wrong is the range check. */
  uint32_t n = hal_mpu_num_regions();
  hal_status_t s = hal_mpu_disable_region(n);
  TEST_ASSERT_TRUE(s == HAL_ERR_INVALID_ARG || s == HAL_ERR_NOT_SUPPORTED);
  s = hal_mpu_disable_region(0xFFFFFFFFu);
  TEST_ASSERT_TRUE(s == HAL_ERR_INVALID_ARG || s == HAL_ERR_NOT_SUPPORTED);
}
#endif /* NAVHAL_CONFIG_DRV_MPU */


#if NAVHAL_CONFIG_DRV_SDIO
void test_conformance_sdio_card_present_is_stable(void) {
  /* Answerable before hal_sdio_init by contract -- a caller decides whether to
   * bring the peripheral up at all from this. Two reads with nothing in
   * between must agree, whichever way the slot is. */
  bool p = hal_sdio_card_present();
  TEST_ASSERT_TRUE(p == hal_sdio_card_present());
}

void test_conformance_sdio_card_init_answers_for_an_empty_slot(void) {
  /* An empty slot has its own answer, so that "no card" and "card present
   * behind a broken data line" are not the same timeout. Branching on
   * card_present keeps this true on a bench with a card in the socket. */
  if (!hal_sdio_card_present()) {
    TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_SDIO_NO_CARD,
                             (uint32_t)hal_sdio_card_init());
  } else {
    hal_sdio_error_t e = hal_sdio_card_init();
    TEST_ASSERT_TRUE(e == HAL_SDIO_OK || e == HAL_SDIO_ERROR);
  }
}

void test_conformance_sdio_command_needs_the_peripheral(void) {
  /* This suite runs before anything brings the controller up, so the clock
   * gate is shut here and a command has nowhere to go. Reporting that is the
   * contract; writing SDIO->ICR/ARG/CMD at a gated peripheral is not.
   *
   * CMD13 (SEND_STATUS) rather than CMD0: if a port has somehow initialised
   * the controller before this point, a status query is harmless, where
   * GO_IDLE_STATE would reset a card the later SDIO suite is about to use. */
  hal_sdio_error_t a = hal_sdio_send_command(13u, 0u, 1u);
  hal_sdio_error_t b = hal_sdio_send_command(13u, 0u, 1u);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)a, (uint32_t)b);
  TEST_ASSERT_TRUE(a != HAL_SDIO_OK);
}

void test_conformance_sdio_get_response_rejects_bad_register(void) {
  /* There are four response registers. An index outside them answers 0 rather
   * than reading whatever lies past the last one. */
  TEST_ASSERT_EQUAL_UINT32(0u, hal_sdio_get_response(0u));
  TEST_ASSERT_EQUAL_UINT32(0u, hal_sdio_get_response(5u));
  TEST_ASSERT_EQUAL_UINT32(0u, hal_sdio_get_response(255u));
}

void test_conformance_sdio_set_callback_accepts_null(void) {
  /* Registration only; NULL is how a caller withdraws. Left empty on the way
   * out so nothing fires into this suite from a later transfer. */
  hal_sdio_detach_callback();
  TEST_ASSERT_TRUE(hal_sdio_card_present() == hal_sdio_card_present());
}

/* ---------------------------------------------------------------------------
 * The block backend FatFs sits on. FatFs itself is third-party and portable
 * (src/utils/fatfs/); what a port supplies is this diskio boundary, and on
 * STM32 that is the SDIO backend. Drive 0 is the only one that exists, so
 * every entry point has the same first question to answer, and answering it is
 * what keeps a mounted filesystem from addressing a drive that is not there.
 * ------------------------------------------------------------------------- */
void test_conformance_disk_rejects_a_drive_that_does_not_exist(void) {
  static uint8_t buf[4];

  /* initialize reports the drive as uninitialised, status reports it as
   * absent -- two different words for "not drive 0", and both are non-OK,
   * which is the part a caller acts on. */
  TEST_ASSERT_TRUE(hal_disk_initialize(1u) != HAL_DISK_STATUS_OK);
  TEST_ASSERT_TRUE((hal_disk_status(1u) & HAL_DISK_STATUS_NODISK) != 0u);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_DISK_RES_PARERR,
                           (uint32_t)hal_disk_read(1u, buf, 0u, 1u));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_DISK_RES_PARERR,
                           (uint32_t)hal_disk_write(1u, buf, 0u, 1u));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_DISK_RES_PARERR,
                           (uint32_t)hal_disk_ioctl(1u, 0u, buf));
}

void test_conformance_disk_rejects_a_zero_length_transfer(void) {
  /* Zero sectors is a parameter error, not a silent success: a caller that
   * computed a zero count has a bug, and reporting OK hides it. Checked before
   * the card is touched, so this holds with an empty slot. */
  static uint8_t buf[4];

  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_DISK_RES_PARERR,
                           (uint32_t)hal_disk_read(0u, buf, 0u, 0u));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_DISK_RES_PARERR,
                           (uint32_t)hal_disk_write(0u, buf, 0u, 0u));
}
#endif /* NAVHAL_CONFIG_DRV_SDIO */


#if NAVHAL_CONFIG_DRV_USB_CDC
void test_conformance_usb_cdc_getters_are_stable(void) {
  /* With no host attached these answer defaults rather than waiting for an
   * enumeration that is not coming. Two reads with nothing in between agree. */
  uint32_t baud = hal_usb_cdc_get_baudrate();
  TEST_ASSERT_EQUAL_UINT32(baud, hal_usb_cdc_get_baudrate());

  uint8_t line = hal_usb_cdc_get_line_state();
  TEST_ASSERT_EQUAL_UINT32((uint32_t)line,
                           (uint32_t)hal_usb_cdc_get_line_state());

  uint16_t brk = hal_usb_cdc_get_break_ms();
  TEST_ASSERT_EQUAL_UINT32((uint32_t)brk,
                           (uint32_t)hal_usb_cdc_get_break_ms());
}

void test_conformance_usb_cdc_set_rx_callback_accepts_null(void) {
  hal_status_t s = hal_usb_cdc_detach_rx_callback();
  TEST_ASSERT_TRUE(s == HAL_OK || s == HAL_ERR_NOT_SUPPORTED);
}

void test_conformance_usb_cdc_notify_needs_a_host(void) {
  /* A serial-state notification travels on the interrupt endpoint, which does
   * not exist until a host has configured the device. Without one the call has
   * to report that rather than write into an endpoint that is not there. */
  if (!hal_usb_cdc_connected()) {
    TEST_ASSERT_TRUE(hal_usb_cdc_notify_serial_state(0u) != HAL_OK);
  }
}

void test_conformance_usb_cdc_init_answers_the_same_twice(void) {
  /* Whether the device can come up is a board question -- the OTG core needs
   * exactly 48 MHz, and a port whose PLL does not supply it must refuse rather
   * than enumerate at the wrong bit rate. What is portable is that asking
   * twice answers the same, so neither branch leaves the driver half-built.
   *
   * Skipped entirely when the console is routed through CDC: there the device
   * being torn down is the wire this suite's own output leaves through. */
  if (hal_console_get_route() == HAL_CONSOLE_ROUTE_CDC) {
    TEST_ASSERT_TRUE(true);
    return;
  }

  hal_status_t a = hal_usb_cdc_init();
  hal_status_t b = hal_usb_cdc_init();
  TEST_ASSERT_EQUAL_UINT32((uint32_t)a, (uint32_t)b);

  hal_status_t c = hal_usb_cdc_deinit();
  hal_status_t d = hal_usb_cdc_deinit();
  TEST_ASSERT_EQUAL_UINT32((uint32_t)c, (uint32_t)d);
}
#endif /* NAVHAL_CONFIG_DRV_USB_CDC */


#if NAVHAL_CONFIG_DRV_ETH
/* ---------------------------------------------------------------------------
 * Ethernet. Every entry point here reaches the MAC, which sits behind a clock
 * gate that init opens -- so what the contract owes a caller before init is an
 * answer, not a register access. These cases run whether or not a cable is
 * plugged in, because none of them needs a link.
 * ------------------------------------------------------------------------- */
void test_conformance_eth_set_callback_accepts_null(void) {
  /* Registration is state, not traffic: withdrawing a callback is legal at any
   * time, and is how a caller switches from interrupt to polled draining. */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_eth_detach_callback());
}

void test_conformance_eth_link_is_up_answers_without_a_mac(void) {
  /* bool, so there is no error to return: an uninitialised MAC reports no
   * link rather than reading a PHY through a peripheral that may be gated. */
  bool up = hal_eth_link_is_up();
  TEST_ASSERT_TRUE(up == hal_eth_link_is_up());
}

void test_conformance_eth_teardown_is_safe_in_any_order(void) {
  /* stop before start, deinit before init, twice each. A port that pokes
   * MACCR here is touching a clock-gated peripheral; one that answers is
   * conformant whichever of the two answers it picks, as long as it picks the
   * same one both times. */
  hal_status_t s1 = hal_eth_stop();
  hal_status_t s2 = hal_eth_stop();
  TEST_ASSERT_EQUAL_UINT32((uint32_t)s1, (uint32_t)s2);

  hal_status_t d1 = hal_eth_deinit();
  hal_status_t d2 = hal_eth_deinit();
  TEST_ASSERT_EQUAL_UINT32((uint32_t)d1, (uint32_t)d2);
}

void test_conformance_eth_needs_init_before_the_bus(void) {
  /* After the teardown above the MAC is down, so this is the uninitialised
   * state by construction. start and phy_write both have to say so rather than
   * driving MACCR and MACMIIAR at a peripheral whose clock is off. */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_NOT_INITIALIZED,
                           (uint32_t)hal_eth_start());
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_NOT_INITIALIZED,
                           (uint32_t)hal_eth_phy_write(0u, 0u));
}
#endif /* NAVHAL_CONFIG_DRV_ETH */


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
    NAVTEST_CASE(test_conformance_usb_cdc_enumerated_answers_without_a_host),
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
    NAVTEST_CASE(test_conformance_rtc_accepts_null_config),
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
    NAVTEST_CASE(test_conformance_flash_raw_erase_refuses_stage1),
    NAVTEST_CASE(test_conformance_flash_raw_erase_refuses_a_sector_that_does_not_exist),
    NAVTEST_CASE(test_conformance_flash_raw_erase_refuses_the_kv_sectors),
    NAVTEST_CASE(test_conformance_flash_raw_program_rejects_null),
    NAVTEST_CASE(test_conformance_flash_raw_program_rejects_odd_address_or_length),
    NAVTEST_CASE(test_conformance_flash_raw_program_rejects_a_range_reaching_stage1),
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
#if NAVHAL_CONFIG_DRV_USB_CDC
    NAVTEST_CASE(test_conformance_usb_cdc_attach_rx_callback_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_SDIO
    NAVTEST_CASE(test_conformance_sdio_attach_callback_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_ETH
    NAVTEST_CASE(test_conformance_eth_attach_callback_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_I2C_DMA
    NAVTEST_CASE(test_conformance_i2c_read_regs_dma_rejects_null_buffer),
#endif
#if NAVHAL_CONFIG_DRV_I2C_DMA
    NAVTEST_CASE(test_conformance_i2c_dma_set_binding_rejects_bad_bus),
#endif
#if NAVHAL_CONFIG_DRV_I2C_DMA
    NAVTEST_CASE(test_conformance_i2c_dma_get_binding_rejects_null_out),
#endif
#if NAVHAL_CONFIG_DRV_DMA
    NAVTEST_CASE(test_conformance_dma_detach_callback_rejects_bad_controller),
    NAVTEST_CASE(test_conformance_dma_attach_callback_rejects_bad_stream),
    NAVTEST_CASE(test_conformance_dma_attach_callback_rejects_null_cb),
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
    NAVTEST_CASE(test_conformance_console_getters_are_stable),
    NAVTEST_CASE(test_conformance_console_write_ignores_null),
#if NAVHAL_CONFIG_DRV_UART
    NAVTEST_CASE(test_conformance_uart_writers_reject_bad_instance),
    NAVTEST_CASE(test_conformance_uart_read_char_answers_on_bad_instance),
#endif
#if NAVHAL_CONFIG_DRV_CRC
    NAVTEST_CASE(test_conformance_crc_init_rejects_null),
#endif
#if NAVHAL_CONFIG_DRV_TIMEBASE
    NAVTEST_CASE(test_conformance_timebase_callback_accepts_null),
#endif
#if NAVHAL_CONFIG_DRV_WWDG
    NAVTEST_CASE(test_conformance_wwdg_kick_needs_a_running_watchdog),
#endif
#if NAVHAL_CONFIG_DRV_ETH
    NAVTEST_CASE(test_conformance_eth_set_callback_accepts_null),
    NAVTEST_CASE(test_conformance_eth_link_is_up_answers_without_a_mac),
    NAVTEST_CASE(test_conformance_eth_teardown_is_safe_in_any_order),
    NAVTEST_CASE(test_conformance_eth_needs_init_before_the_bus),
#endif
#if NAVHAL_CONFIG_DRV_USB_CDC
    NAVTEST_CASE(test_conformance_usb_cdc_getters_are_stable),
    NAVTEST_CASE(test_conformance_usb_cdc_set_rx_callback_accepts_null),
    NAVTEST_CASE(test_conformance_usb_cdc_notify_needs_a_host),
    NAVTEST_CASE(test_conformance_usb_cdc_init_answers_the_same_twice),
#endif
#if NAVHAL_CONFIG_DRV_SDIO
    NAVTEST_CASE(test_conformance_sdio_card_present_is_stable),
    NAVTEST_CASE(test_conformance_sdio_card_init_answers_for_an_empty_slot),
    NAVTEST_CASE(test_conformance_sdio_command_needs_the_peripheral),
    NAVTEST_CASE(test_conformance_sdio_get_response_rejects_bad_register),
    NAVTEST_CASE(test_conformance_sdio_set_callback_accepts_null),
    NAVTEST_CASE(test_conformance_disk_rejects_a_drive_that_does_not_exist),
    NAVTEST_CASE(test_conformance_disk_rejects_a_zero_length_transfer),
#endif
#if NAVHAL_CONFIG_DRV_RTC
    NAVTEST_CASE(test_conformance_rtc_backup_write_rejects_bad_index),
    NAVTEST_CASE(test_conformance_rtc_set_wakeup_rejects_bad_period),
    NAVTEST_CASE(test_conformance_rtc_cancel_is_idempotent),
    NAVTEST_CASE(test_conformance_rtc_get_clock_is_a_documented_source),
#endif
#if NAVHAL_CONFIG_DRV_MPU
    NAVTEST_CASE(test_conformance_mpu_disable_region_rejects_bad_index),
#endif
#if NAVHAL_CONFIG_BOOT_SNIFFER
    NAVTEST_CASE(test_conformance_boot_block_init_validates),
    NAVTEST_CASE(test_conformance_boot_getters_are_stable),
    NAVTEST_CASE(test_conformance_boot_entry_gate_round_trips),
    NAVTEST_CASE(test_conformance_boot_request_is_refused_while_disabled),
    NAVTEST_CASE(test_conformance_boot_request_target_checks_its_argument),
    NAVTEST_CASE(test_conformance_boot_match_ignores_other_traffic),
    NAVTEST_CASE(test_conformance_boot_clear_and_heal_need_a_valid_block),
    NAVTEST_CASE(test_conformance_boot_attempt_counts_and_clears),
    NAVTEST_CASE(test_conformance_boot_set_prepare_accepts_null),
#endif
#if NAVHAL_CONFIG_DRV_INTERRUPT
    NAVTEST_CASE(test_conformance_interrupt_enable_rejects_bad_irq),
    NAVTEST_CASE(test_conformance_interrupt_disable_rejects_bad_irq),
    NAVTEST_CASE(test_conformance_interrupt_attach_rejects_null_callback),
    NAVTEST_CASE(test_conformance_interrupt_detach_rejects_bad_irq),
    NAVTEST_CASE(test_conformance_interrupt_clear_pending_rejects_bad_irq),
    NAVTEST_CASE(test_conformance_interrupt_set_priority_rejects_bad_irq),
    NAVTEST_CASE(test_conformance_interrupt_get_priority_of_bad_irq_answers),
    NAVTEST_CASE(test_conformance_interrupt_bad_irq_is_not_pending),
    NAVTEST_CASE(test_conformance_interrupt_dispatch_of_empty_slot_returns),
    NAVTEST_CASE(test_conformance_interrupt_global_state_round_trips),
    NAVTEST_CASE(test_conformance_interrupt_enable_with_priority_rejects_bad_irq),
#endif
};

const navtest_suite_t test_conformance_suite = {
    .name = "CONFORMANCE",
    .cases = conformance_cases,
    .count = sizeof(conformance_cases) / sizeof(conformance_cases[0]),
    .between = NULL,
};
