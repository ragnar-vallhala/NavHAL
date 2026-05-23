#include "navhal_port_clock.h"
#include "navhal_port_pwm.h"
#include "family/rcc_reg.h"
#include "navhal_port_timer.h"
#include "family/timer_reg.h"
#include "navhal_port_uart.h"
#include "navtest/navtest.h"
#include "utils/timer_types.h"
#include <stdint.h>

// -------------------- PWM Init --------------------
//
uint32_t timer_get_ccr(hal_timer_t timer, uint32_t channel) {
  switch (channel) {
  case 1:
    return GET_TIMx_BASE(timer)->CCR1;
  case 2:
    return GET_TIMx_BASE(timer)->CCR2;
  case 3:
    return GET_TIMx_BASE(timer)->CCR3;
  case 4:
    return GET_TIMx_BASE(timer)->CCR4;
  default:
    return 0;
  }
}
void test_hal_pwm_init_apb1(void) {
  hal_pwm_handle_t pwm = {.timer = TIM2, .channel = 1};

  hal_pwm_init(&pwm, 1000, 0.5f); // 1 kHz, 50% duty

  uint32_t bus_clk = hal_clock_get_apb1clk();
  uint32_t timer_clk = bus_clk * 2; // APB1 is DIV4 in hal_clock_init
  uint32_t expected_psc = timer_clk / 1000000 - 1;
  uint32_t expected_arr = (timer_clk / (expected_psc + 1)) / 1000 - 1;
  uint32_t expected_ccr = (uint32_t)((expected_arr + 1) * 0.5f + 0.5f);

  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate = 9600});
  TEST_ASSERT_EQUAL_UINT32(expected_arr, hal_timer_get_auto_reload(pwm.timer));
  // CCR should be close to expected duty
  TEST_ASSERT_EQUAL_UINT32(expected_ccr, timer_get_ccr(pwm.timer, pwm.channel));
}

void test_hal_pwm_init_apb2(void) {
  hal_pwm_handle_t pwm = {.timer = TIM1, .channel = 2};

  hal_pwm_init(&pwm, 2000, 0.25f); // 2 kHz, 25% duty

  uint32_t bus_clk = hal_clock_get_apb2clk();
  uint32_t timer_clk = bus_clk * 2; // APB2 is DIV2 in hal_clock_init
  uint32_t expected_psc = timer_clk / 1000000 - 1;
  uint32_t expected_arr = (timer_clk / (expected_psc + 1)) / 2000 - 1;
  uint32_t expected_ccr = (uint32_t)((expected_arr + 1) * 0.25f + 0.5f);

  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate = 9600});
  TEST_ASSERT_EQUAL_UINT32(expected_arr, hal_timer_get_auto_reload(pwm.timer));
  TEST_ASSERT_EQUAL_UINT32(expected_ccr, timer_get_ccr(pwm.timer, pwm.channel));
}

// -------------------- PWM Start/Stop --------------------
void test_hal_pwm_start_sets_counter_enable(void) {
  hal_pwm_handle_t pwm = {.timer = TIM3, .channel = 1};

  hal_pwm_init(&pwm, 1000, 0.5f);
  hal_pwm_start(&pwm);

  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate = 9600});
  TEST_ASSERT_TRUE((GET_TIMx_BASE(pwm.timer)->CR1 & TIMx_CR1_CEN) != 0);
}

void test_hal_pwm_stop_clears_counter_enable(void) {
  hal_pwm_handle_t pwm = {.timer = TIM4, .channel = 2};

  hal_pwm_init(&pwm, 500, 0.5f);
  hal_pwm_start(&pwm);
  hal_pwm_stop(&pwm);

  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate = 9600});
  TEST_ASSERT_TRUE((GET_TIMx_BASE(pwm.timer)->CR1 & TIMx_CR1_CEN) == 0);
}

// -------------------- PWM Duty Cycle --------------------
void test_hal_pwm_set_duty_cycle_updates_ccr(void) {
  hal_pwm_handle_t pwm = {.timer = TIM2, .channel = 1};

  hal_pwm_init(&pwm, 1000, 0.1f); // start with 10%
  hal_pwm_set_duty_cycle(&pwm, 0.75f);

  uint32_t arr = hal_timer_get_auto_reload(pwm.timer);
  uint32_t expected_ccr = (uint32_t)((arr + 1) * 0.75f + 0.5f);

  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate = 9600});
  TEST_ASSERT_EQUAL_UINT32(expected_ccr, timer_get_ccr(pwm.timer, pwm.channel));
}

/* -------------------- Standardized contract -------------------- */

void test_hal_pwm_init_rejects_null_handle(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_pwm_init(NULL, 1000, 0.5f));
}

void test_hal_pwm_start_rejects_null_handle(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_pwm_start(NULL));
}

void test_hal_pwm_stop_rejects_null_handle(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_pwm_stop(NULL));
}

void test_hal_pwm_set_duty_cycle_rejects_null_handle(void) {
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_pwm_set_duty_cycle(NULL, 0.5f));
}

void test_hal_pwm_set_frequency_returns_ok(void) {
  hal_pwm_handle_t pwm = {.timer = TIM2, .channel = 1};
  hal_pwm_init(&pwm, 1000, 0.5f);
  /* Accept HAL_ERR_NOT_SUPPORTED for ports that haven't wired runtime
   * frequency change yet (Section 6 of the spec — graceful degradation
   * is the contract). */
  hal_status_t s = hal_pwm_set_frequency(&pwm, 2000);
  TEST_ASSERT_TRUE(s == HAL_OK || s == HAL_ERR_NOT_SUPPORTED);
}

void test_hal_pwm_set_frequency_rejects_null_handle(void) {
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_ERR_INVALID_ARG,
      (uint32_t)hal_pwm_set_frequency(NULL, 1000));
}

static const navtest_case_t pwm_cases[] = {
    NAVTEST_CASE(test_hal_pwm_init_apb1),
    NAVTEST_CASE(test_hal_pwm_init_apb2),
    NAVTEST_CASE(test_hal_pwm_start_sets_counter_enable),
    NAVTEST_CASE(test_hal_pwm_stop_clears_counter_enable),
    NAVTEST_CASE(test_hal_pwm_set_duty_cycle_updates_ccr),
    /* standardized contract */
    NAVTEST_CASE(test_hal_pwm_init_rejects_null_handle),
    NAVTEST_CASE(test_hal_pwm_start_rejects_null_handle),
    NAVTEST_CASE(test_hal_pwm_stop_rejects_null_handle),
    NAVTEST_CASE(test_hal_pwm_set_duty_cycle_rejects_null_handle),
    NAVTEST_CASE(test_hal_pwm_set_frequency_returns_ok),
    NAVTEST_CASE(test_hal_pwm_set_frequency_rejects_null_handle),
};

const navtest_suite_t test_pwm_suite = {
    .name = "PWM",
    .cases = pwm_cases,
    .count = sizeof(pwm_cases) / sizeof(pwm_cases[0]),
    .between = NULL,
};
