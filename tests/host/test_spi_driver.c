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
 * @file test_spi_driver.c
 * @brief Deep host (SIL) tests for spi_f7.c (the F7 SPI driver) against
 *        simulated MMIO. Verifies the F7-specific CR2.DS/FRXTH data-size config
 *        (the F4 CR1.DFF is gone) and the byte-DR transfer loop with pre-seeded
 *        SR flags.
 */

#include "host_mmio.h"
#include "navhal_port_spi.h"
#include "family/spi_reg.h"
#include "navtest/navtest.h"
#include <stdint.h>

static volatile SPI_Reg_Typedef *s(hal_spi_instance_t inst) {
  return (volatile SPI_Reg_Typedef *)GET_SPIx_BASE((uint8_t)inst);
}

void test_host_spi_init_cr1_fields(void) {
  host_mmio_reset();
  hal_spi_config_t cfg = {.baudrate = HAL_SPI_BAUDRATE_DIV16,
                          .cpol = HAL_SPI_CPOL_HIGH,
                          .cpha = HAL_SPI_CPHA_2EDGE,
                          .datasize = HAL_SPI_DATASIZE_8BIT,
                          .firstbit = HAL_SPI_FIRSTBIT_LSB};
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_spi_init(HAL_SPI_1, &cfg));
  uint32_t cr1 = s(HAL_SPI_1)->CR1;
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_SPI_BAUDRATE_DIV16 << SPI_CR1_BR_Pos,
                           cr1 & SPI_CR1_BR_Msk);
  TEST_ASSERT_BITS_HIGH(SPI_CR1_CPOL | SPI_CR1_CPHA | SPI_CR1_LSBFIRST |
                            SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_SPE,
                        cr1);
}

void test_host_spi_init_cr2_datasize_8bit(void) {
  host_mmio_reset();
  hal_spi_config_t cfg = {.baudrate = HAL_SPI_BAUDRATE_DIV8,
                          .cpol = HAL_SPI_CPOL_LOW,
                          .cpha = HAL_SPI_CPHA_1EDGE,
                          .datasize = HAL_SPI_DATASIZE_8BIT,
                          .firstbit = HAL_SPI_FIRSTBIT_MSB};
  hal_spi_init(HAL_SPI_1, &cfg);
  /* 8-bit: DS = 0111 and FRXTH set; CPOL/CPHA/LSB clear. */
  TEST_ASSERT_EQUAL_UINT32(SPI_CR2_DS_8BIT, s(HAL_SPI_1)->CR2 & SPI_CR2_DS_Msk);
  TEST_ASSERT_BITS_HIGH(SPI_CR2_FRXTH, s(HAL_SPI_1)->CR2);
  TEST_ASSERT_BITS_LOW(SPI_CR1_CPOL | SPI_CR1_CPHA | SPI_CR1_LSBFIRST,
                       s(HAL_SPI_1)->CR1);
}

void test_host_spi_init_cr2_datasize_16bit(void) {
  host_mmio_reset();
  hal_spi_config_t cfg = {.baudrate = HAL_SPI_BAUDRATE_DIV4,
                          .cpol = HAL_SPI_CPOL_LOW,
                          .cpha = HAL_SPI_CPHA_1EDGE,
                          .datasize = HAL_SPI_DATASIZE_16BIT,
                          .firstbit = HAL_SPI_FIRSTBIT_MSB};
  hal_spi_init(HAL_SPI_1, &cfg);
  TEST_ASSERT_EQUAL_UINT32(SPI_CR2_DS_16BIT, s(HAL_SPI_1)->CR2 & SPI_CR2_DS_Msk);
  /* FRXTH is for byte access only — not set for 16-bit. */
  TEST_ASSERT_BITS_LOW(SPI_CR2_FRXTH, s(HAL_SPI_1)->CR2);
}

void test_host_spi_init_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_spi_init(HAL_SPI_1, NULL));
}

void test_host_spi_transmit_writes_dr(void) {
  host_mmio_reset();
  /* TXE + RXNE so the TX/drain loop and the trailing BSY wait all pass. */
  host_reg_set((uintptr_t)&s(HAL_SPI_1)->SR, SPI_SR_TXE | SPI_SR_RXNE);
  uint8_t data[] = {0x12, 0x34, 0x56};
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_OK,
      (uint32_t)hal_spi_transmit(HAL_SPI_1, data, sizeof(data), 0));
  /* DR is byte-accessed; its low byte holds the last frame written. */
  TEST_ASSERT_EQUAL_UINT32(0x56u, s(HAL_SPI_1)->DR & 0xFFu);
}

void test_host_spi_transmit_receive_round_trips_dr(void) {
  host_mmio_reset();
  host_reg_set((uintptr_t)&s(HAL_SPI_1)->SR, SPI_SR_TXE | SPI_SR_RXNE);
  uint8_t tx[] = {0xA1, 0xB2};
  uint8_t rx[2] = {0};
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_OK,
      (uint32_t)hal_spi_transmit_receive(HAL_SPI_1, tx, rx, 2, 0));
  /* The single DR location loops the written frame back to the reader. */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)tx[1], (uint32_t)rx[1]);
}

void test_host_spi_rejects_null_data(void) {
  TEST_ASSERT_TRUE(hal_spi_transmit(HAL_SPI_1, NULL, 4, 0) != HAL_OK);
  TEST_ASSERT_TRUE(hal_spi_receive(HAL_SPI_1, NULL, 4, 0) != HAL_OK);
}

NAVTEST_CASE_DECL(test_host_spi_init_cr1_fields);
NAVTEST_CASE_DECL(test_host_spi_init_cr2_datasize_8bit);
NAVTEST_CASE_DECL(test_host_spi_init_cr2_datasize_16bit);
NAVTEST_CASE_DECL(test_host_spi_init_rejects_null);
NAVTEST_CASE_DECL(test_host_spi_transmit_writes_dr);
NAVTEST_CASE_DECL(test_host_spi_transmit_receive_round_trips_dr);
NAVTEST_CASE_DECL(test_host_spi_rejects_null_data);

static const navtest_case_t spi_driver_cases[] = {
    NAVTEST_CASE(test_host_spi_init_cr1_fields),
    NAVTEST_CASE(test_host_spi_init_cr2_datasize_8bit),
    NAVTEST_CASE(test_host_spi_init_cr2_datasize_16bit),
    NAVTEST_CASE(test_host_spi_init_rejects_null),
    NAVTEST_CASE(test_host_spi_transmit_writes_dr),
    NAVTEST_CASE(test_host_spi_transmit_receive_round_trips_dr),
    NAVTEST_CASE(test_host_spi_rejects_null_data),
};

const navtest_suite_t test_spi_driver_suite = {
    .name = "SPI DRIVER (host)",
    .cases = spi_driver_cases,
    .count = sizeof(spi_driver_cases) / sizeof(spi_driver_cases[0]),
    .between = NULL,
};
