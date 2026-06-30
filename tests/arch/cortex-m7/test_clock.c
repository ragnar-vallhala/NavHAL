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
 * @file tests/arch/cortex-m7/test_clock.c
 * @brief STM32F7 hal_clock_* white-box tests (clock_f7.c).
 *
 * Differences from the F4 suite: the console UART is USART3 (drained via the
 * F7 ISR/TC bit, re-initialised after each reconfigure), and the PLL is sourced
 * from HSI — the Nucleo-F767ZI's HSE-from-ST-LINK-MCO availability is not
 * assumed here (F7 bring-up runs on HSI), so the F4 HSE cases are omitted.
 */

#include "test_clock.h"
#include "navhal_port_clock.h"
#include "family/rcc_reg.h"
#include "navhal_port_uart.h"
#include "family/uart_reg.h"
#include "navtest/navtest.h"
#include "utils/clock_types.h"
#include <stdint.h>

/* The console (USART3) baud depends on the bus clock; drain TX (F7 ISR.TC)
 * before any reconfigure so in-flight characters are not corrupted. */
static void wait_uart_empty(void) {
  volatile UARTx_Reg_Typedef *uart3 =
      (volatile UARTx_Reg_Typedef *)(GET_USARTx_BASE(3));
  while (!(uart3->ISR & USART_ISR_TC))
    ;
}

static void reinit_console(void) {
  hal_uart_init(HAL_UART_3, &(hal_uart_config_t){.baudrate = 9600});
}

/* HSI(16 MHz)/8 * 168 / 2 = 168 MHz — ≤180, so no over-drive needed. */
static const hal_pll_config_t k_pll = {.input_src = HAL_CLOCK_SOURCE_HSI,
                                       .pll_m = 8,
                                       .pll_n = 168,
                                       .pll_p = 2,
                                       .pll_q = 7};

// -------------------- Clock Initialization --------------------
void test_hal_clock_init_hsi(void) {
  hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_HSI};
  wait_uart_empty();
  hal_clock_init(&cfg, NULL);
  reinit_console();
  TEST_ASSERT_EQUAL_UINT32(0, (RCC->CFGR >> RCC_CFGR_SWS_BIT) & 0x3);
}

void test_hal_clock_init_pll(void) {
  hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_PLL};
  wait_uart_empty();
  hal_clock_init(&cfg, &k_pll);
  reinit_console();
  TEST_ASSERT_EQUAL_UINT32(2, (RCC->CFGR >> RCC_CFGR_SWS_BIT) & 0x3);
}

// -------------------- SYSCLK --------------------
void test_hal_clock_get_sysclk_returns_correct_value_hsi(void) {
  hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_HSI};
  wait_uart_empty();
  hal_clock_init(&cfg, NULL);
  reinit_console();
  TEST_ASSERT_EQUAL_UINT32(16000000, hal_clock_get_sysclk());
}

void test_hal_clock_get_sysclk_returns_correct_value_pll(void) {
  hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_PLL};
  wait_uart_empty();
  hal_clock_init(&cfg, &k_pll);
  reinit_console();
  uint32_t expected = (16000000 / k_pll.pll_m) * k_pll.pll_n / k_pll.pll_p;
  TEST_ASSERT_EQUAL_UINT32(expected, hal_clock_get_sysclk());
}

// -------------------- AHB / APB getters --------------------
/* Temporarily reprogram a bus prescaler, read the getter, restore CFGR before
 * the result is reported so the console UART stays at a valid baud. */
void test_hal_clock_get_ahbclk_returns_correct_value(void) {
  wait_uart_empty();
  uint32_t saved = RCC->CFGR;
  RCC->CFGR &= ~RCC_CFGR_HPRE_MASK; /* AHB /1 */
  uint32_t expected = hal_clock_get_sysclk();
  uint32_t result = hal_clock_get_ahbclk();
  RCC->CFGR = saved;
  TEST_ASSERT_EQUAL_UINT32(expected, result);
}

void test_hal_clock_get_apb1clk_returns_correct_value(void) {
  wait_uart_empty();
  uint32_t saved = RCC->CFGR;
  RCC->CFGR &= ~RCC_CFGR_PPRE1_MASK;
  RCC->CFGR |= (0x5 << RCC_CFGR_PPRE1_BIT); /* APB1 /4 */
  uint32_t expected = hal_clock_get_sysclk() / 4;
  uint32_t result = hal_clock_get_apb1clk();
  RCC->CFGR = saved;
  TEST_ASSERT_EQUAL_UINT32(expected, result);
}

void test_hal_clock_get_apb2clk_returns_correct_value(void) {
  wait_uart_empty();
  uint32_t saved = RCC->CFGR;
  RCC->CFGR &= ~RCC_CFGR_PPRE2_MASK;
  RCC->CFGR |= (0x4 << RCC_CFGR_PPRE2_BIT); /* APB2 /2 */
  uint32_t expected = hal_clock_get_sysclk() / 2;
  uint32_t result = hal_clock_get_apb2clk();
  RCC->CFGR = saved;
  TEST_ASSERT_EQUAL_UINT32(expected, result);
}

/* -------------------- Status-return contract -------------------- */
void test_hal_clock_init_returns_ok_for_hsi(void) {
  hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_HSI};
  wait_uart_empty();
  hal_status_t s = hal_clock_init(&cfg, NULL);
  reinit_console();
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)s);
}

void test_hal_clock_init_rejects_null_cfg(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_clock_init(NULL, NULL));
}

void test_hal_clock_init_pll_rejects_null_pll_cfg(void) {
  hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_PLL};
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_clock_init(&cfg, NULL));
}

NAVTEST_CASE_DECL(test_hal_clock_init_hsi);
NAVTEST_CASE_DECL(test_hal_clock_init_pll);
NAVTEST_CASE_DECL(test_hal_clock_get_sysclk_returns_correct_value_hsi);
NAVTEST_CASE_DECL(test_hal_clock_get_sysclk_returns_correct_value_pll);
NAVTEST_CASE_DECL(test_hal_clock_get_ahbclk_returns_correct_value);
NAVTEST_CASE_DECL(test_hal_clock_get_apb1clk_returns_correct_value);
NAVTEST_CASE_DECL(test_hal_clock_get_apb2clk_returns_correct_value);
NAVTEST_CASE_DECL(test_hal_clock_init_returns_ok_for_hsi);
NAVTEST_CASE_DECL(test_hal_clock_init_rejects_null_cfg);
NAVTEST_CASE_DECL(test_hal_clock_init_pll_rejects_null_pll_cfg);


static const navtest_case_t clock_cases[] = {
    NAVTEST_CASE(test_hal_clock_init_hsi),
    NAVTEST_CASE(test_hal_clock_init_pll),
    NAVTEST_CASE(test_hal_clock_get_sysclk_returns_correct_value_hsi),
    NAVTEST_CASE(test_hal_clock_get_sysclk_returns_correct_value_pll),
    NAVTEST_CASE(test_hal_clock_get_ahbclk_returns_correct_value),
    NAVTEST_CASE(test_hal_clock_get_apb1clk_returns_correct_value),
    NAVTEST_CASE(test_hal_clock_get_apb2clk_returns_correct_value),
    /* status-return contract — success + error paths */
    NAVTEST_CASE(test_hal_clock_init_returns_ok_for_hsi),
    NAVTEST_CASE(test_hal_clock_init_rejects_null_cfg),
    NAVTEST_CASE(test_hal_clock_init_pll_rejects_null_pll_cfg),
};

const navtest_suite_t test_clock_suite = {
    .name = "CLOCK",
    .cases = clock_cases,
    .count = sizeof(clock_cases) / sizeof(clock_cases[0]),
    /* Reconfiguring the clock corrupts the UART baud — flush after each. */
    .between = wait_uart_empty,
};
