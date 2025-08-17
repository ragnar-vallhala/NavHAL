#include "test_clock.h"
#include "core/cortex-m4/clock.h"
#include "core/cortex-m4/rcc_reg.h"
#include "core/cortex-m4/uart.h"
#include "unity.h"
#include "utils/clock_types.h"
#include <stdint.h>

// -------------------- Clock Initialization --------------------
void test_hal_clock_init_hsi(void) {
  hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_HSI,
                            .hpre_div = RCC_CFGR_HPRE_DIV1,
                            .ppre1_div = RCC_CFGR_PPRE_DIV1,
                            .ppre2_div = RCC_CFGR_PPRE_DIV1};

  hal_clock_init(&cfg, NULL);
  uart2_init(9600);
  TEST_ASSERT_EQUAL_UINT32(0, (RCC->CFGR >> RCC_CFGR_SWS_BIT) & 0x3);
}

void test_hal_clock_init_hse(void) {
  hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_HSE,
                            .hpre_div = RCC_CFGR_HPRE_DIV1,
                            .ppre1_div = RCC_CFGR_PPRE_DIV1,
                            .ppre2_div = RCC_CFGR_PPRE_DIV1};
  hal_clock_init(&cfg, NULL);
  uart2_init(9600);
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
  hal_clock_init(&cfg, &pll_cfg);
  uart2_init(9600);
  TEST_ASSERT_EQUAL_UINT32(2, (RCC->CFGR >> RCC_CFGR_SWS_BIT) & 0x3);
}

// -------------------- SYSCLK --------------------
void test_hal_clock_get_sysclk_returns_correct_value_hsi(void) {
  hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_HSI,
                            .hpre_div = RCC_CFGR_HPRE_DIV1,
                            .ppre1_div = RCC_CFGR_PPRE_DIV1,
                            .ppre2_div = RCC_CFGR_PPRE_DIV1};
  hal_clock_init(&cfg, NULL);
  uint32_t sysclk = hal_clock_get_sysclk();
  uart2_init(9600);
  TEST_ASSERT_EQUAL_UINT32(16000000, sysclk);
}

void test_hal_clock_get_sysclk_returns_correct_value_hse(void) {
  hal_clock_config_t cfg = {.source = HAL_CLOCK_SOURCE_HSE,
                            .hpre_div = RCC_CFGR_HPRE_DIV1,
                            .ppre1_div = RCC_CFGR_PPRE_DIV1,
                            .ppre2_div = RCC_CFGR_PPRE_DIV1};
  hal_clock_init(&cfg, NULL);
  uint32_t sysclk = hal_clock_get_sysclk();
  uart2_init(9600);
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
  hal_clock_init(&cfg, &pll_cfg);
  uint32_t sysclk = hal_clock_get_sysclk();
  uint32_t expected = (8000000 / pll_cfg.pll_m) * pll_cfg.pll_n / pll_cfg.pll_p;
  uart2_init(9600);
  TEST_ASSERT_EQUAL_UINT32(expected, sysclk);
}

// -------------------- AHB / APB --------------------
void test_hal_clock_get_ahbclk_returns_correct_value(void) {
  RCC->CFGR &= ~RCC_CFGR_HPRE_MASK;
  RCC->CFGR |= (0x0 << RCC_CFGR_HPRE_BIT); // divide by 1
  TEST_ASSERT_EQUAL_UINT32(hal_clock_get_sysclk(), hal_clock_get_ahbclk());
}

void test_hal_clock_get_apb1clk_returns_correct_value(void) {
  RCC->CFGR &= ~RCC_CFGR_PPRE1_MASK;
  RCC->CFGR |= (0x5 << RCC_CFGR_PPRE1_BIT); // divide by 4
  TEST_ASSERT_EQUAL_UINT32(hal_clock_get_sysclk() / 4, hal_clock_get_apb1clk());
}

void test_hal_clock_get_apb2clk_returns_correct_value(void) {
  RCC->CFGR &= ~RCC_CFGR_PPRE2_MASK;
  RCC->CFGR |= (0x4 << RCC_CFGR_PPRE2_BIT); // divide by 2
  TEST_ASSERT_EQUAL_UINT32(hal_clock_get_sysclk() / 2, hal_clock_get_apb2clk());
}
