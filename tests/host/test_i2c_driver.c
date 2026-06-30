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
 * @file test_i2c_driver.c
 * @brief Deep host (SIL) tests for i2c_f7.c (the F7 timing-register I2C IP)
 *        against simulated MMIO. The transfer FSM polls ISR (TXIS/RXNE/STOPF/
 *        NACKF); those flags are pre-seeded so the loops terminate, and the
 *        resulting CR2 framing + data registers are asserted.
 */

#include "host_mmio.h"
#include "navhal_port_i2c.h"
#include "family/i2c_reg.h"
#include "family/rcc_reg.h"
#include "common/hal_i2c.h"
#include "navtest/navtest.h"
#include <stdint.h>

static volatile I2C_Reg_Typedef *bus(hal_i2c_bus_t b) {
  return (volatile I2C_Reg_Typedef *)I2C_GET_BASE(b);
}
static void seed_isr(hal_i2c_bus_t b, uint32_t bits) {
  host_reg_set((uintptr_t)&bus(b)->ISR, bits);
}

/* -------------------- init -------------------- */

void test_host_i2c_init_standard_timing(void) {
  host_mmio_reset();
  hal_i2c_config_t cfg = {.clock_speed = HAL_I2C_SPEED_STANDARD,
                          .own_address = I2C_MASTER};
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_i2c_init(HAL_I2C_1, &cfg));
  TEST_ASSERT_EQUAL_UINT32(I2C_TIMINGR_SM_16MHZ, bus(HAL_I2C_1)->TIMINGR);
  TEST_ASSERT_BITS_HIGH(I2C_CR1_PE, bus(HAL_I2C_1)->CR1);
  TEST_ASSERT_BITS_HIGH(I2C_APB1ENR_MASK(0), RCC->APB1ENR);
}

void test_host_i2c_init_fast_timing(void) {
  host_mmio_reset();
  hal_i2c_config_t cfg = {.clock_speed = HAL_I2C_SPEED_FAST,
                          .own_address = I2C_MASTER};
  hal_i2c_init(HAL_I2C_2, &cfg);
  TEST_ASSERT_EQUAL_UINT32(I2C_TIMINGR_FM_16MHZ, bus(HAL_I2C_2)->TIMINGR);
  TEST_ASSERT_BITS_HIGH(I2C_CR1_PE, bus(HAL_I2C_2)->CR1);
}

void test_host_i2c_init_rejects_slave(void) {
  host_mmio_reset();
  hal_i2c_config_t cfg = {.clock_speed = HAL_I2C_SPEED_STANDARD,
                          .own_address = 0x42};
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_IO,
                           (uint32_t)hal_i2c_init(HAL_I2C_3, &cfg));
}

void test_host_i2c_init_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_i2c_init(HAL_I2C_3, NULL));
}

/* -------------------- write transfer framing -------------------- */

void test_host_i2c_write_frames_cr2_and_data(void) {
  host_mmio_reset();
  seed_isr(HAL_I2C_1, I2C_ISR_TXIS | I2C_ISR_STOPF);
  uint8_t data[] = {0xDE, 0xAD};
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_OK,
      (uint32_t)hal_i2c_write(HAL_I2C_1, 0x50, data, sizeof(data)));
  uint32_t cr2 = bus(HAL_I2C_1)->CR2;
  TEST_ASSERT_EQUAL_UINT32((uint32_t)(0x50u << 1), cr2 & 0xFEu);   /* SADD */
  TEST_ASSERT_EQUAL_UINT32(2u, (cr2 >> 16) & 0xFFu);              /* NBYTES */
  TEST_ASSERT_BITS_HIGH(I2C_CR2_AUTOEND | I2C_CR2_START, cr2);
  TEST_ASSERT_BITS_LOW(I2C_CR2_RD_WRN, cr2);                       /* write */
  TEST_ASSERT_EQUAL_UINT32(0xADu, bus(HAL_I2C_1)->TXDR & 0xFFu);   /* last byte */
}

void test_host_i2c_write_nack_returns_io(void) {
  host_mmio_reset();
  seed_isr(HAL_I2C_1, I2C_ISR_NACKF); /* device does not ACK */
  uint8_t b = 0x11;
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_IO,
                           (uint32_t)hal_i2c_write(HAL_I2C_1, 0x50, &b, 1));
  TEST_ASSERT_BITS_HIGH(I2C_ICR_NACKCF, bus(HAL_I2C_1)->ICR);
}

/* -------------------- read transfer -------------------- */

void test_host_i2c_read_sets_rd_wrn_and_returns_data(void) {
  host_mmio_reset();
  bus(HAL_I2C_1)->RXDR = 0x5A;
  seed_isr(HAL_I2C_1, I2C_ISR_RXNE | I2C_ISR_STOPF);
  uint8_t out[1] = {0};
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_i2c_read(HAL_I2C_1, 0x50, out, 1));
  TEST_ASSERT_BITS_HIGH(I2C_CR2_RD_WRN, bus(HAL_I2C_1)->CR2);
  TEST_ASSERT_EQUAL_UINT32(0x5Au, out[0]);
}

void test_host_i2c_rejects_null_data(void) {
  TEST_ASSERT_TRUE(hal_i2c_write(HAL_I2C_1, 0x50, NULL, 4) != HAL_OK);
  TEST_ASSERT_TRUE(hal_i2c_read(HAL_I2C_1, 0x50, NULL, 4) != HAL_OK);
}

NAVTEST_CASE_DECL(test_host_i2c_init_standard_timing);
NAVTEST_CASE_DECL(test_host_i2c_init_fast_timing);
NAVTEST_CASE_DECL(test_host_i2c_init_rejects_slave);
NAVTEST_CASE_DECL(test_host_i2c_init_rejects_null);
NAVTEST_CASE_DECL(test_host_i2c_write_frames_cr2_and_data);
NAVTEST_CASE_DECL(test_host_i2c_write_nack_returns_io);
NAVTEST_CASE_DECL(test_host_i2c_read_sets_rd_wrn_and_returns_data);
NAVTEST_CASE_DECL(test_host_i2c_rejects_null_data);

static const navtest_case_t i2c_driver_cases[] = {
    NAVTEST_CASE(test_host_i2c_init_standard_timing),
    NAVTEST_CASE(test_host_i2c_init_fast_timing),
    NAVTEST_CASE(test_host_i2c_init_rejects_slave),
    NAVTEST_CASE(test_host_i2c_init_rejects_null),
    NAVTEST_CASE(test_host_i2c_write_frames_cr2_and_data),
    NAVTEST_CASE(test_host_i2c_write_nack_returns_io),
    NAVTEST_CASE(test_host_i2c_read_sets_rd_wrn_and_returns_data),
    NAVTEST_CASE(test_host_i2c_rejects_null_data),
};

const navtest_suite_t test_i2c_driver_suite = {
    .name = "I2C DRIVER (host)",
    .cases = i2c_driver_cases,
    .count = sizeof(i2c_driver_cases) / sizeof(i2c_driver_cases[0]),
    .between = NULL,
};
