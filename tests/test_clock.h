#ifndef TEST_CLOCK_H
#define TEST_CLOCK_H

#include "navtest/navtest.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
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

// -------------------- Status-return contract --------------------
void test_hal_clock_init_returns_ok_for_hsi(void);
void test_hal_clock_init_rejects_null_cfg(void);
void test_hal_clock_init_pll_rejects_null_pll_cfg(void);

extern const navtest_suite_t test_clock_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_CLOCK_H
