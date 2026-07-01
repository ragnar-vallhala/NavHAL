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
 * @file test_uart_driver.c
 * @brief Deep host (SIL) tests for uart_f7.c (the F7 USART driver) against
 *        simulated MMIO. Reset RCC reads back as HSI 16 MHz with /1 buses, so
 *        BRR == round(16e6 / baud). Transfer flags (ISR.TXE/RXNE) are
 *        pre-seeded so the polling loops terminate.
 */

#include "host_mmio.h"
#include "navhal_port_uart.h"
#include "family/uart_reg.h"
#include "family/rcc_reg.h"
#include "navtest/navtest.h"
#include <stdint.h>

static volatile UARTx_Reg_Typedef *u(hal_uart_t inst) {
  return (volatile UARTx_Reg_Typedef *)GET_USARTx_BASE(inst);
}

void test_host_uart_init_brr_usart3_115200(void) {
  host_mmio_reset();
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_OK,
      (uint32_t)hal_uart_init(HAL_UART_3, &(hal_uart_config_t){.baudrate = 115200}));
  /* 16 MHz APB1 (reset HSI, /1): BRR = round(16e6/115200) = 139. */
  TEST_ASSERT_EQUAL_UINT32(139u, u(HAL_UART_3)->BRR);
  /* UE | TE | RE enabled. */
  TEST_ASSERT_BITS_HIGH(USART_CR1_UE | USART_CR1_TE | USART_CR1_RE,
                        u(HAL_UART_3)->CR1);
  /* USART3 clock (APB1ENR bit 18). */
  TEST_ASSERT_BITS_HIGH(RCC_APB1ENR_USART3EN, RCC->APB1ENR);
}

void test_host_uart_init_brr_various_bauds(void) {
  const uint32_t bauds[] = {9600, 19200, 38400, 57600, 115200};
  for (unsigned i = 0; i < 5; i++) {
    host_mmio_reset();
    hal_uart_init(HAL_UART_3, &(hal_uart_config_t){.baudrate = bauds[i]});
    uint32_t expect = (16000000u + bauds[i] / 2) / bauds[i];
    TEST_ASSERT_EQUAL_UINT32(expect, u(HAL_UART_3)->BRR);
  }
}

void test_host_uart_init_usart1_uses_apb2_clock(void) {
  host_mmio_reset();
  hal_uart_init(HAL_UART_1, &(hal_uart_config_t){.baudrate = 9600});
  TEST_ASSERT_EQUAL_UINT32((16000000u + 4800u) / 9600u, u(HAL_UART_1)->BRR);
  TEST_ASSERT_BITS_HIGH(RCC_APB2ENR_USART1EN, RCC->APB2ENR);
}

void test_host_uart_init_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_uart_init(HAL_UART_3, NULL));
}

void test_host_uart_write_char_to_tdr(void) {
  host_mmio_reset();
  hal_uart_init(HAL_UART_3, &(hal_uart_config_t){.baudrate = 9600});
  host_reg_set((uintptr_t)&u(HAL_UART_3)->ISR, USART_ISR_TXE); /* TX empty */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_uart_write_char(HAL_UART_3, 'Z'));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)'Z', u(HAL_UART_3)->TDR & 0xFFu);
}

void test_host_uart_read_char_from_rdr(void) {
  host_mmio_reset();
  hal_uart_init(HAL_UART_3, &(hal_uart_config_t){.baudrate = 9600});
  u(HAL_UART_3)->RDR = 0x41;
  host_reg_set((uintptr_t)&u(HAL_UART_3)->ISR, USART_ISR_RXNE);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)'A',
                           (uint32_t)(uint8_t)hal_uart_read_char(HAL_UART_3));
}

void test_host_uart_read_char_clears_errors_via_icr(void) {
  host_mmio_reset();
  hal_uart_init(HAL_UART_3, &(hal_uart_config_t){.baudrate = 9600});
  host_reg_set((uintptr_t)&u(HAL_UART_3)->ISR, USART_ISR_ORE); /* overrun */
  char c = hal_uart_read_char(HAL_UART_3);
  TEST_ASSERT_EQUAL_UINT32(0u, (uint32_t)(uint8_t)c); /* returns 0 on error */
  /* ICR got the overrun-clear bit written. */
  TEST_ASSERT_BITS_HIGH(USART_ICR_ORECF, u(HAL_UART_3)->ICR);
}

void test_host_uart_available_reflects_rxne(void) {
  host_mmio_reset();
  hal_uart_init(HAL_UART_3, &(hal_uart_config_t){.baudrate = 9600});
  TEST_ASSERT_FALSE(hal_uart_available(HAL_UART_3));
  host_reg_set((uintptr_t)&u(HAL_UART_3)->ISR, USART_ISR_RXNE);
  TEST_ASSERT_TRUE(hal_uart_available(HAL_UART_3));
}

void test_host_uart_enable_interrupt_sets_cr1(void) {
  host_mmio_reset();
  hal_uart_init(HAL_UART_3, &(hal_uart_config_t){.baudrate = 9600});
  hal_uart_enable_interrupt(HAL_UART_3, 1, 0);
  TEST_ASSERT_BITS_HIGH(USART_CR1_RXNEIE, u(HAL_UART_3)->CR1);
  TEST_ASSERT_BITS_LOW(USART_CR1_TXEIE, u(HAL_UART_3)->CR1);
}

NAVTEST_CASE_DECL(test_host_uart_init_brr_usart3_115200);
NAVTEST_CASE_DECL(test_host_uart_init_brr_various_bauds);
NAVTEST_CASE_DECL(test_host_uart_init_usart1_uses_apb2_clock);
NAVTEST_CASE_DECL(test_host_uart_init_rejects_null);
NAVTEST_CASE_DECL(test_host_uart_write_char_to_tdr);
NAVTEST_CASE_DECL(test_host_uart_read_char_from_rdr);
NAVTEST_CASE_DECL(test_host_uart_read_char_clears_errors_via_icr);
NAVTEST_CASE_DECL(test_host_uart_available_reflects_rxne);
NAVTEST_CASE_DECL(test_host_uart_enable_interrupt_sets_cr1);

static const navtest_case_t uart_driver_cases[] = {
    NAVTEST_CASE(test_host_uart_init_brr_usart3_115200),
    NAVTEST_CASE(test_host_uart_init_brr_various_bauds),
    NAVTEST_CASE(test_host_uart_init_usart1_uses_apb2_clock),
    NAVTEST_CASE(test_host_uart_init_rejects_null),
    NAVTEST_CASE(test_host_uart_write_char_to_tdr),
    NAVTEST_CASE(test_host_uart_read_char_from_rdr),
    NAVTEST_CASE(test_host_uart_read_char_clears_errors_via_icr),
    NAVTEST_CASE(test_host_uart_available_reflects_rxne),
    NAVTEST_CASE(test_host_uart_enable_interrupt_sets_cr1),
};

const navtest_suite_t test_uart_driver_suite = {
    .name = "UART DRIVER (host)",
    .cases = uart_driver_cases,
    .count = sizeof(uart_driver_cases) / sizeof(uart_driver_cases[0]),
    .between = NULL,
};
