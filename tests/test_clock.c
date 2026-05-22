#include "test_clock.h"
#include "navhal_port_clock.h"
#include "family/rcc_reg.h"
#include "navhal_port_uart.h"
#include "navtest/navtest.h"
#include "utils/clock_types.h"
#include <stdint.h>

static void wait_uart_empty(void) {
  volatile UARTx_Reg_Typedef *uart2 =
      (volatile UARTx_Reg_Typedef *)(GET_USARTx_BASE(2));
  while (!(uart2->SR & USART_SR_TC))
    ;
}

// -------------------- Clock Initialization --------------------
void test_hal_clock_init_hsi(void) {
  hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_HSI,
                            .hpre_div = RCC_CFGR_HPRE_DIV1,
                            .ppre1_div = RCC_CFGR_PPRE_DIV1,
                            .ppre2_div = RCC_CFGR_PPRE_DIV1};

  wait_uart_empty();
  hal_clock_init(&cfg, NULL);

  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=9600});

  TEST_ASSERT_EQUAL_UINT32(0, (RCC->CFGR >> RCC_CFGR_SWS_BIT) & 0x3);
}

void test_hal_clock_init_hse(void) {
  hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_HSE,
                            .hpre_div = RCC_CFGR_HPRE_DIV1,
                            .ppre1_div = RCC_CFGR_PPRE_DIV1,
                            .ppre2_div = RCC_CFGR_PPRE_DIV1};
  wait_uart_empty();
  hal_clock_init(&cfg, NULL);

  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=9600});
  TEST_ASSERT_EQUAL_UINT32(1, (RCC->CFGR >> RCC_CFGR_SWS_BIT) & 0x3);
}

void test_hal_clock_init_pll(void) {
  hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_PLL,
                            .hpre_div = RCC_CFGR_HPRE_DIV1,
                            .ppre1_div = RCC_CFGR_PPRE_DIV4,
                            .ppre2_div = RCC_CFGR_PPRE_DIV2};
  hal_pll_config_t pll_cfg = {.input_src = HAL_CLOCK_SOURCE_HSE,
                              .pll_m = 8,
                              .pll_n = 168,
                              .pll_p = 2,
                              .pll_q = 7};
  wait_uart_empty();
  hal_clock_init(&cfg, &pll_cfg);

  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=9600});
  TEST_ASSERT_EQUAL_UINT32(2, (RCC->CFGR >> RCC_CFGR_SWS_BIT) & 0x3);
}

// -------------------- SYSCLK --------------------
void test_hal_clock_get_sysclk_returns_correct_value_hsi(void) {
  hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_HSI,
                            .hpre_div = RCC_CFGR_HPRE_DIV1,
                            .ppre1_div = RCC_CFGR_PPRE_DIV1,
                            .ppre2_div = RCC_CFGR_PPRE_DIV1};
  wait_uart_empty();
  hal_clock_init(&cfg, NULL);

  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=9600});
  uint32_t sysclk = hal_clock_get_sysclk();
  TEST_ASSERT_EQUAL_UINT32(16000000, sysclk);
}

void test_hal_clock_get_sysclk_returns_correct_value_hse(void) {
  hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_HSE,
                            .hpre_div = RCC_CFGR_HPRE_DIV1,
                            .ppre1_div = RCC_CFGR_PPRE_DIV1,
                            .ppre2_div = RCC_CFGR_PPRE_DIV1};
  wait_uart_empty();
  hal_clock_init(&cfg, NULL);

  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=9600});
  uint32_t sysclk = hal_clock_get_sysclk();
  TEST_ASSERT_EQUAL_UINT32(8000000, sysclk);
}

void test_hal_clock_get_sysclk_returns_correct_value_pll(void) {
  hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_PLL,
                            .hpre_div = RCC_CFGR_HPRE_DIV1,
                            .ppre1_div = RCC_CFGR_PPRE_DIV4,
                            .ppre2_div = RCC_CFGR_PPRE_DIV2};
  hal_pll_config_t pll_cfg = {.input_src = HAL_CLOCK_SOURCE_HSE,
                              .pll_m = 8,
                              .pll_n = 168,
                              .pll_p = 2,
                              .pll_q = 7};
  wait_uart_empty();
  hal_clock_init(&cfg, &pll_cfg);

  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=9600});
  uint32_t sysclk = hal_clock_get_sysclk();
  uint32_t expected = (8000000 / pll_cfg.pll_m) * pll_cfg.pll_n / pll_cfg.pll_p;
  TEST_ASSERT_EQUAL_UINT32(expected, sysclk);
}

// -------------------- AHB / APB --------------------
/*
 * These tests temporarily reprogram the AHB/APB bus prescalers to verify the
 * clock getters. The APB1 bus clocks USART2 (the test console UART), so
 * RCC->CFGR is saved and restored *before* the assertion is reported — the
 * pass/fail output then leaves at the original, correct baud rate.
 */
void test_hal_clock_get_ahbclk_returns_correct_value(void) {
  wait_uart_empty();
  uint32_t saved = RCC->CFGR;
  RCC->CFGR &= ~RCC_CFGR_HPRE_MASK;
  RCC->CFGR |= (0x0 << RCC_CFGR_HPRE_BIT); // divide by 1
  uint32_t expected = hal_clock_get_sysclk();
  uint32_t result = hal_clock_get_ahbclk();
  RCC->CFGR = saved; // restore bus prescalers before reporting
  TEST_ASSERT_EQUAL_UINT32(expected, result);
}

void test_hal_clock_get_apb1clk_returns_correct_value(void) {
  wait_uart_empty();
  uint32_t saved = RCC->CFGR;
  RCC->CFGR &= ~RCC_CFGR_PPRE1_MASK;
  RCC->CFGR |= (0x5 << RCC_CFGR_PPRE1_BIT); // divide by 4
  uint32_t expected = hal_clock_get_sysclk() / 4;
  uint32_t result = hal_clock_get_apb1clk();
  RCC->CFGR = saved; // restore APB1 prescaler — keeps the console UART valid
  TEST_ASSERT_EQUAL_UINT32(expected, result);
}

void test_hal_clock_get_apb2clk_returns_correct_value(void) {
  wait_uart_empty();
  uint32_t saved = RCC->CFGR;
  RCC->CFGR &= ~RCC_CFGR_PPRE2_MASK;
  RCC->CFGR |= (0x4 << RCC_CFGR_PPRE2_BIT); // divide by 2
  uint32_t expected = hal_clock_get_sysclk() / 2;
  uint32_t result = hal_clock_get_apb2clk();
  RCC->CFGR = saved; // restore bus prescalers before reporting
  TEST_ASSERT_EQUAL_UINT32(expected, result);
}

/* -------------------- Status-return contract -------------------- */

void test_hal_clock_init_returns_ok_for_hsi(void) {
  hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_HSI,
                            .hpre_div = RCC_CFGR_HPRE_DIV1,
                            .ppre1_div = RCC_CFGR_PPRE_DIV1,
                            .ppre2_div = RCC_CFGR_PPRE_DIV1};
  wait_uart_empty();
  hal_status_t s = hal_clock_init(&cfg, NULL);
  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=9600});
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK, (uint32_t)s);
}

void test_hal_clock_init_rejects_null_cfg(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_clock_init(NULL, NULL));
}

void test_hal_clock_init_pll_rejects_null_pll_cfg(void) {
  hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_PLL,
                            .hpre_div = RCC_CFGR_HPRE_DIV1,
                            .ppre1_div = RCC_CFGR_PPRE_DIV1,
                            .ppre2_div = RCC_CFGR_PPRE_DIV1};
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_clock_init(&cfg, NULL));
}

static const navtest_case_t clock_cases[] = {
    NAVTEST_CASE(test_hal_clock_init_hsi),
    NAVTEST_CASE(test_hal_clock_init_hse),
    NAVTEST_CASE(test_hal_clock_init_pll),
    NAVTEST_CASE(test_hal_clock_get_sysclk_returns_correct_value_hsi),
    NAVTEST_CASE(test_hal_clock_get_sysclk_returns_correct_value_hse),
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
