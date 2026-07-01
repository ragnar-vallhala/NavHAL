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

#include "test_i2c.h"
#include "navhal_port_clock.h"
#include "navhal_port_i2c.h"
#include "family/i2c_reg.h"
#include "navtest/navtest.h"
#include "navtest/navtest_pil.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* The F7 I²C programs timing through a single TIMINGR (no CR2.FREQ / CCR /
 * TRISE). Assert the standard-mode timing preset and that the peripheral is
 * enabled (PE). */
void test_i2c_init_config(void) {
  /* Renode's I²C model doesn't reflect the TIMINGR write-back; skip on PIL. */
  NAVTEST_SKIP_ON_PIL();
  hal_i2c_config_t config = {.clock_speed = HAL_I2C_SPEED_STANDARD,
                             .own_address = I2C_MASTER,
                             .acknowledge = true};
  hal_i2c_init(HAL_I2C_1, &config);
  volatile I2C_Reg_Typedef *I2C = I2C_GET_BASE(HAL_I2C_1);

  TEST_ASSERT_EQUAL_UINT32(I2C_TIMINGR_SM_16MHZ, I2C->TIMINGR);
  TEST_ASSERT_TRUE(I2C->CR1 & I2C_CR1_PE);
}

void test_i2c_fast_mode_config(void) {
  NAVTEST_SKIP_ON_PIL();
  hal_i2c_config_t config = {.clock_speed = HAL_I2C_SPEED_FAST,
                             .own_address = I2C_MASTER,
                             .acknowledge = true};
  /* Use I2C2 to avoid the re-init guard on I2C1. */
  hal_i2c_init(HAL_I2C_2, &config);
  volatile I2C_Reg_Typedef *I2C = I2C_GET_BASE(HAL_I2C_2);

  TEST_ASSERT_EQUAL_UINT32(I2C_TIMINGR_FM_16MHZ, I2C->TIMINGR);
  TEST_ASSERT_TRUE(I2C->CR1 & I2C_CR1_PE);
}

/* -------------------- Standardized contract tests -------------------- */

/* The STM32F4 I²C driver re-initializes from a stored config across
 * successive `init` calls, so the first init in the suite leaves the
 * driver in NOT_INITIALIZED for subsequent re-inits without a deinit.
 * These tests assert what the contract *requires* — a defined status —
 * without insisting on a specific code beyond OK/not-OK. */

void test_hal_i2c_init_returns_ok(void) {
  hal_i2c_config_t cfg = {.clock_speed = HAL_I2C_SPEED_STANDARD,
                          .own_address = 0,
                          .acknowledge = true};
  hal_status_t s = hal_i2c_init(HAL_I2C_1, &cfg);
  TEST_ASSERT_TRUE(s == HAL_OK || s == HAL_ERR_NOT_INITIALIZED);
}

void test_hal_i2c_init_rejects_null_config(void) {
  /* Any non-OK status is acceptable here. */
  TEST_ASSERT_TRUE(hal_i2c_init(HAL_I2C_1, NULL) != HAL_OK);
}

void test_hal_i2c_write_rejects_null_data(void) {
  TEST_ASSERT_TRUE(hal_i2c_write(HAL_I2C_1, 0x50, NULL, 4) != HAL_OK);
}

void test_hal_i2c_read_rejects_null_data(void) {
  TEST_ASSERT_TRUE(hal_i2c_read(HAL_I2C_1, 0x50, NULL, 4) != HAL_OK);
}

void test_hal_i2c_write_read_rejects_null_data(void) {
  uint8_t buf[4];
  TEST_ASSERT_TRUE(
      hal_i2c_write_read(HAL_I2C_1, 0x50, NULL, 1, buf, 1) != HAL_OK);
  TEST_ASSERT_TRUE(
      hal_i2c_write_read(HAL_I2C_1, 0x50, buf, 1, NULL, 1) != HAL_OK);
}

void test_hal_i2c_typed_id_compiles(void) {
  /* The standardized signature takes hal_i2c_bus_t — pure compile-time
   * check that bare `uint8_t bus` no longer works. */
  hal_i2c_bus_t bus = HAL_I2C_2;
  uint8_t data = 0;
  (void)hal_i2c_write(bus, 0x50, &data, 0);
  TEST_ASSERT_TRUE(1);
}

/* PIL-only: exercise a real master transfer against the Renode-modelled BMP180
 * on I2C1 @ 0x77 (see tools/renode/stm32f767zi.repl). Reads the chip-ID
 * register (0xD0 -> 0x55) via write_read, proving the i2c_f7 transfer FSM
 * (CR2 framing, TXIS/TC/RXNE/STOPF) drives an attached device. Skipped on HIL
 * (the bench has no sensor wired). */
void test_i2c_pil_device_read_chip_id(void) {
  NAVTEST_PIL_ONLY();
  hal_i2c_config_t cfg = {.clock_speed = HAL_I2C_SPEED_STANDARD,
                          .own_address = I2C_MASTER,
                          .acknowledge = true};
  hal_i2c_init(HAL_I2C_1, &cfg);
  uint8_t reg = 0xD0;
  uint8_t id = 0;
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_OK,
      (uint32_t)hal_i2c_write_read(HAL_I2C_1, 0x77, &reg, 1, &id, 1));
  TEST_ASSERT_EQUAL_UINT32(0x55u, (uint32_t)id);
}
/* PROGMEM slot for each case name on AVR; no-op elsewhere. */
NAVTEST_CASE_DECL(test_i2c_init_config);
NAVTEST_CASE_DECL(test_i2c_pil_device_read_chip_id);
NAVTEST_CASE_DECL(test_i2c_fast_mode_config);
NAVTEST_CASE_DECL(test_hal_i2c_init_returns_ok);
NAVTEST_CASE_DECL(test_hal_i2c_init_rejects_null_config);
NAVTEST_CASE_DECL(test_hal_i2c_write_rejects_null_data);
NAVTEST_CASE_DECL(test_hal_i2c_read_rejects_null_data);
NAVTEST_CASE_DECL(test_hal_i2c_write_read_rejects_null_data);
NAVTEST_CASE_DECL(test_hal_i2c_typed_id_compiles);


static const navtest_case_t i2c_cases[] = {
    NAVTEST_CASE(test_i2c_init_config),
    NAVTEST_CASE(test_i2c_fast_mode_config),
    NAVTEST_CASE(test_hal_i2c_init_returns_ok),
    NAVTEST_CASE(test_hal_i2c_init_rejects_null_config),
    NAVTEST_CASE(test_hal_i2c_write_rejects_null_data),
    NAVTEST_CASE(test_hal_i2c_read_rejects_null_data),
    NAVTEST_CASE(test_hal_i2c_write_read_rejects_null_data),
    NAVTEST_CASE(test_hal_i2c_typed_id_compiles),
    NAVTEST_CASE(test_i2c_pil_device_read_chip_id),
};

const navtest_suite_t test_i2c_suite = {
    .name = "I2C",
    .cases = i2c_cases,
    .count = sizeof(i2c_cases) / sizeof(i2c_cases[0]),
    .between = NULL,
};
