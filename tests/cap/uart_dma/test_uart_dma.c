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
 * @file tests/cap/uart_dma/test_uart_dma.c
 * @brief On-target tests for the DMA-backed UART TX path.
 *
 * @details
 * DMA-transmits a marker on the navtest console (USART3 on the F767). Its
 * appearance in the captured UART log is the physical proof that the F7 DMA
 * TX path actually drives the USART — a stub returning HAL_OK could not put
 * bytes on the wire. Plus the argument-contract checks. Runs where
 * NAVHAL_CONFIG_DRV_UART_DMA is set.
 */

#include "test_uart_dma.h"
#include "common/hal_features.h"
#include "navhal.h"
#include "navtest/navtest.h"
#include "navtest_target.h"

#if NAVHAL_CONFIG_DRV_UART_DMA

void test_uart_dma_tx_writes(void) {
  hal_status_t st =
      hal_uart_write_string_dma(NAVTEST_UART, "[uart-dma-tx-ok]\r\n");
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)st);
  /* Let the DMA move the bytes and the USART shift them out before the suite
   * continues to use the same console. */
  for (volatile uint32_t i = 0; i < 3000000u; i++)
    __asm__ volatile("nop");
}

void test_uart_dma_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_uart_write_dma(NAVTEST_UART, NULL, 4));
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_uart_init_dma_rx(NAVTEST_UART, NULL, 4));
}

/* PROGMEM slot for each case name on AVR; no-op elsewhere (UART DMA is
 * Cortex-M only). */
NAVTEST_CASE_DECL(test_uart_dma_tx_writes);
NAVTEST_CASE_DECL(test_uart_dma_rejects_null);

static const navtest_case_t uart_dma_cases[] = {
    NAVTEST_CASE(test_uart_dma_tx_writes),
    NAVTEST_CASE(test_uart_dma_rejects_null),
};

const navtest_suite_t test_uart_dma_suite = {
    .name = "UART DMA (cap)",
    .cases = uart_dma_cases,
    .count = sizeof(uart_dma_cases) / sizeof(uart_dma_cases[0]),
    .between = NULL,
};

#endif /* NAVHAL_CONFIG_DRV_UART_DMA */
