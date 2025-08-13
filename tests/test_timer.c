#include "core/cortex-m4/timer.h"
#include "unity.h"
#include <stdint.h>

#define TEST_TIMER TIM2
#define TEST_PSC 83
#define TEST_ARR 999
#define TEST_CHANNEL 1
#define TEST_COMPARE_VALUE 500

void test_timer_init_sets_prescaler_and_arr(void) {
  timer_init(TEST_TIMER, TEST_PSC, TEST_ARR);

  volatile uint32_t *psc =
      (volatile uint32_t *)(TIM2_BASE + TIM_GP1_PSC_OFFSET);
  volatile uint32_t *arr =
      (volatile uint32_t *)(TIM2_BASE + TIM_GP1_ARR_OFFSET);

  TEST_ASSERT_EQUAL_UINT32(TEST_PSC, *psc);
  TEST_ASSERT_EQUAL_UINT32(TEST_ARR, *arr);
}

void test_timer_start_sets_CEN_bit(void) {
  timer_start(TEST_TIMER);

  volatile uint32_t *cr1 =
      (volatile uint32_t *)(TIM2_BASE + TIM_GP1_CR1_OFFSET);
  TEST_ASSERT_BITS_HIGH(1 << TIM_GP1_CR1_CEN_BIT, *cr1);
}

void test_timer_stop_clears_CEN_bit(void) {
  timer_stop(TEST_TIMER);

  volatile uint32_t *cr1 =
      (volatile uint32_t *)(TIM2_BASE + TIM_GP1_CR1_OFFSET);
  TEST_ASSERT_BITS_LOW(1 << TIM_GP1_CR1_CEN_BIT, *cr1);
}

void test_timer_reset_clears_count(void) {
  // Set count to a non-zero value
  volatile uint32_t *cnt =
      (volatile uint32_t *)(TIM2_BASE + TIM_GP1_CNT_OFFSET);
  *cnt = 12345;

  timer_reset(TEST_TIMER);

  TEST_ASSERT_EQUAL_UINT32(0, *cnt);
}

void test_timer_set_compare_and_get_compare(void) {
  timer_set_compare(TEST_TIMER, TEST_CHANNEL, TEST_COMPARE_VALUE);

  volatile uint32_t *ccr = (volatile uint32_t *)(TIM2_BASE + TIMx_CCR1_OFFSET);
  TEST_ASSERT_EQUAL_UINT32(TEST_COMPARE_VALUE, *ccr);

  uint32_t compare_val = timer_get_compare(TEST_TIMER, TEST_CHANNEL);
  TEST_ASSERT_EQUAL_UINT32(TEST_COMPARE_VALUE, compare_val);
}

void test_timer_enable_and_disable_channel(void) {
  timer_disable_channel(TEST_TIMER, TEST_CHANNEL);

  volatile uint32_t *ccer = (volatile uint32_t *)(TIM2_BASE + TIMx_CCER_OFFSET);
  TEST_ASSERT_BITS_LOW(1 << TIMx_CCER_CC1E_BIT, *ccer);

  timer_enable_channel(TEST_TIMER, TEST_CHANNEL);
  TEST_ASSERT_BITS_HIGH(1 << TIMx_CCER_CC1E_BIT, *ccer);
}

void test_timer_enable_and_disable_interrupt(void) {
  volatile uint32_t *dier =
      (volatile uint32_t *)(TIM2_BASE + TIM_GP1_DIER_OFFSET);

  timer_enable_interrupt(TEST_TIMER);
  TEST_ASSERT_BITS_HIGH(1 << TIM_GP1_DIER_UIE_BIT, *dier);
}

void test_timer_clear_interrupt_flag_clears_UIF(void) {
  volatile uint32_t *sr = (volatile uint32_t *)(TIM2_BASE + TIM_GP1_SR_OFFSET);
  *sr |= (1 << TIM_GP1_SR_UIF_BIT);

  timer_clear_interrupt_flag(TEST_TIMER);
  TEST_ASSERT_BITS_LOW(1 << TIM_GP1_SR_UIF_BIT, *sr);
}

void test_timer_get_arr_returns_arr_value(void) {
  volatile uint32_t *arr =
      (volatile uint32_t *)(TIM2_BASE + TIM_GP1_ARR_OFFSET);
  *arr = TEST_ARR;

  uint32_t arr_val = timer_get_arr(TEST_TIMER, 1);
  TEST_ASSERT_EQUAL_UINT32(TEST_ARR, arr_val);
}

void test_timer_get_count_returns_count_value(void) {
  volatile uint32_t *cnt =
      (volatile uint32_t *)(TIM2_BASE + TIM_GP1_CNT_OFFSET);
  *cnt = 1234;

  uint32_t count_val = timer_get_count(TEST_TIMER);
  TEST_ASSERT_EQUAL_UINT32(1234, count_val);
}
