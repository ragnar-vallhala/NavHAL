#ifndef TEST_CLOCK_H
#define TEST_CLOCK_H

#include <stdint.h>

// -------------------- Clock Initialization --------------------
void test_hal_clock_init_hsi(void);
void test_hal_clock_init_hse(void);
void test_hal_clock_init_pll(void);

// -------------------- SYSCLK --------------------
void test_hal_clock_get_sysclk_returns_correct_value_hsi(void);
void test_hal_clock_get_sysclk_returns_correct_value_hse(void);
void test_hal_clock_get_sysclk_returns_correct_value_pll(void);

// -------------------- AHB / APB --------------------
void test_hal_clock_get_ahbclk_returns_correct_value(void);
void test_hal_clock_get_apb1clk_returns_correct_value(void);
void test_hal_clock_get_apb2clk_returns_correct_value(void);

#endif // TEST_CLOCK_H
