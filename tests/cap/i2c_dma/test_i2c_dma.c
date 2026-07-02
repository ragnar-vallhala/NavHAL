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
 * @file tests/cap/i2c_dma/test_i2c_dma.c
 * @brief Contract tests for the DMA-backed I²C register read (STM32F7).
 *
 * @details
 * A completed DMA read requires a device to answer on the bus, so it belongs in
 * a wired sample (samples/cortex-m/19_hal_dma_i2c), not in this suite — the
 * committed tests run on a bare board with nothing attached. What is checked
 * here is the argument contract of hal_i2c_read_regs_dma. Runs where
 * NAVHAL_CONFIG_DRV_I2C_DMA is set.
 */

#include "test_i2c_dma.h"

#if NAVHAL_CONFIG_DRV_I2C_DMA

#include "navhal.h"
#include "navtest/navtest.h"
#include <stdint.h>

static void i2c_dma_rx_complete(void) {}

void test_i2c_dma_rejects_null_cfg(void) {
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_i2c_read_regs_dma(HAL_I2C_1, 0x50, 0x00, NULL,
                                      i2c_dma_rx_complete));
}

/* PROGMEM slot for each case name on AVR; no-op elsewhere (I²C DMA is
 * Cortex-M only). */
NAVTEST_CASE_DECL(test_i2c_dma_rejects_null_cfg);

static const navtest_case_t i2c_dma_cases[] = {
    NAVTEST_CASE(test_i2c_dma_rejects_null_cfg),
};

const navtest_suite_t test_i2c_dma_suite = {
    .name = "I2C DMA (cap)",
    .cases = i2c_dma_cases,
    .count = sizeof(i2c_dma_cases) / sizeof(i2c_dma_cases[0]),
    .between = NULL,
};

#endif /* NAVHAL_CONFIG_DRV_I2C_DMA */
