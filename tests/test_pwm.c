#include "core/cortex-m4/clock.h"
#include "core/cortex-m4/pwm.h"
#include "core/cortex-m4/rcc_reg.h"
#include "core/cortex-m4/timer.h"
#include "core/cortex-m4/timer_reg.h"
#include "core/cortex-m4/uart.h"
#include "unity.h"
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
  PWM_Handle pwm = {.timer = TIM2, .channel = 1};

  hal_pwm_init(&pwm, 1000, 0.5f); // 1 kHz, 50% duty

  uint32_t bus_clk = hal_clock_get_apb1clk();
  uint32_t expected_psc = bus_clk / 1000000 - 1;
  uint32_t expected_arr = (bus_clk / (expected_psc + 1)) / 1000 - 1;
  uint32_t expected_ccr = (uint32_t)((expected_arr + 1) * 0.5f + 0.5f);

  uart2_init(9600);
  TEST_ASSERT_EQUAL_UINT32(expected_arr, timer_get_arr(pwm.timer, pwm.channel));
  // CCR should be close to expected duty
  TEST_ASSERT_EQUAL_UINT32(expected_ccr, timer_get_ccr(pwm.timer, pwm.channel));
}

void test_hal_pwm_init_apb2(void) {
  PWM_Handle pwm = {.timer = TIM1, .channel = 2};

  hal_pwm_init(&pwm, 2000, 0.25f); // 2 kHz, 25% duty

  uint32_t bus_clk = hal_clock_get_apb2clk();
  uint32_t expected_psc = bus_clk / 1000000 - 1;
  uint32_t expected_arr = (bus_clk / (expected_psc + 1)) / 2000 - 1;
  uint32_t expected_ccr = (uint32_t)((expected_arr + 1) * 0.25f + 0.5f);

  uart2_init(9600);
  TEST_ASSERT_EQUAL_UINT32(expected_arr, timer_get_arr(pwm.timer, pwm.channel));
  TEST_ASSERT_EQUAL_UINT32(expected_ccr, timer_get_ccr(pwm.timer, pwm.channel));
}

// -------------------- PWM Start/Stop --------------------
void test_hal_pwm_start_sets_counter_enable(void) {
  PWM_Handle pwm = {.timer = TIM3, .channel = 1};

  hal_pwm_init(&pwm, 1000, 0.5f);
  hal_pwm_start(&pwm);

  uart2_init(9600);
  TEST_ASSERT_TRUE((GET_TIMx_BASE(pwm.timer)->CR1 & TIMx_CR1_CEN) != 0);
}

void test_hal_pwm_stop_clears_counter_enable(void) {
  PWM_Handle pwm = {.timer = TIM4, .channel = 2};

  hal_pwm_init(&pwm, 500, 0.5f);
  hal_pwm_start(&pwm);
  hal_pwm_stop(&pwm);

  uart2_init(9600);
  TEST_ASSERT_TRUE((GET_TIMx_BASE(pwm.timer)->CR1 & TIMx_CR1_CEN) == 0);
}

// -------------------- PWM Duty Cycle --------------------
void test_hal_pwm_set_duty_cycle_updates_ccr(void) {
  PWM_Handle pwm = {.timer = TIM2, .channel = 1};

  hal_pwm_init(&pwm, 1000, 0.1f); // start with 10%
  hal_pwm_set_duty_cycle(&pwm, 0.75f);

  uint32_t arr = timer_get_arr(pwm.timer, pwm.channel);
  uint32_t expected_ccr = (uint32_t)((arr + 1) * 0.75f + 0.5f);

  uart2_init(9600);
  TEST_ASSERT_EQUAL_UINT32(expected_ccr, timer_get_ccr(pwm.timer, pwm.channel));
}

