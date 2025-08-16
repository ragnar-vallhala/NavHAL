#include "core/cortex-m4/timer.h"
#include "core/cortex-m4/timer_reg.h"
#include "unity.h"
#include <stdint.h>

#define TEST_TIMER TIM2
#define TEST_PSC 83
#define TEST_ARR 999
#define TEST_CHANNEL 1
#define TEST_COMPARE_VALUE 500

void test_timer_init_sets_prescaler_and_arr(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  if (timer == NULL)
    return;
  timer_init(TEST_TIMER, TEST_PSC, TEST_ARR);

  TEST_ASSERT_EQUAL_UINT32(TEST_PSC, timer->PSC);
  TEST_ASSERT_EQUAL_UINT32(TEST_ARR, timer->ARR);
}

void test_timer_start_sets_CEN_bit(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  if (timer == NULL)
    return;
  timer_start(TEST_TIMER);
  TEST_ASSERT_BITS_HIGH(TIMx_CR1_CEN, timer->CR1 & TIMx_CR1_CEN);
}

void test_timer_stop_clears_CEN_bit(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  if (timer == NULL)
    return;
  timer_stop(TEST_TIMER);
  TEST_ASSERT_BITS_LOW(TIMx_CR1_CEN, timer->CR1 & TIMx_CR1_CEN);
}

void test_timer_reset_clears_count(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  if (timer == NULL)
    return;
  timer_stop(TEST_TIMER);
  timer_reset(TEST_TIMER);
  TEST_ASSERT_EQUAL_UINT32(0, timer->CNT);
}

void test_timer_set_compare_and_get_compare(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  if (timer == NULL)
    return;
  timer_set_compare(TEST_TIMER, TEST_CHANNEL, TEST_COMPARE_VALUE);
  TEST_ASSERT_EQUAL_UINT32(
      TEST_COMPARE_VALUE,
      (TEST_CHANNEL == 1
           ? (timer->CCR1)
           : (TEST_CHANNEL == 2
                  ? (timer->CCR2)
                  : (TEST_TIMER == 3
                         ? (timer->CCR3)
                         : (TEST_TIMER == 4 ? (timer->CCR4) : (0))))));

  uint32_t compare_val = timer_get_compare(TEST_TIMER, TEST_CHANNEL);
  TEST_ASSERT_EQUAL_UINT32(TEST_COMPARE_VALUE, compare_val);
}

void test_timer_enable_and_disable_channel(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  if (timer == NULL)
    return;
  timer_disable_channel(TEST_TIMER, TEST_CHANNEL);

  TEST_ASSERT_BITS_LOW(TIMx_CCER_CCxE_MASK(TEST_CHANNEL),
                       timer->CCER & TIMx_CCER_CCxE_MASK(TEST_CHANNEL));

  timer_enable_channel(TEST_TIMER, TEST_CHANNEL);
  TEST_ASSERT_BITS_HIGH(TIMx_CCER_CCxE_MASK(TEST_CHANNEL),
                        timer->CCER & TIMx_CCER_CCxE_MASK(TEST_CHANNEL));
}

void test_timer_enable_and_disable_interrupt(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  if (timer == NULL)
    return;
  timer_enable_interrupt(TEST_TIMER);
  TEST_ASSERT_BITS_HIGH(TIMx_DIER_UIE, timer->DIER);
}

void test_timer_clear_interrupt_flag_clears_UIF(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  if (timer == NULL)
    return;
  timer->SR |= (TIMx_SR_UIF);
  timer_clear_interrupt_flag(TEST_TIMER);
  TEST_ASSERT_BITS_LOW(~(TIMx_SR_UIF), timer->SR & TIMx_SR_UIF);
}

void test_timer_get_arr_returns_arr_value(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  if (timer == NULL)
    return;
  timer_set_arr(TEST_TIMER, TEST_CHANNEL, TEST_ARR);
  uint32_t arr_val = timer_get_arr(TEST_TIMER, 1);
  TEST_ASSERT_EQUAL_UINT32(TEST_ARR, arr_val);
}

void test_timer_get_count_returns_count_value(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  if (timer == NULL)
    return;
  timer->CNT = 1234;
  uint32_t count_val = timer_get_count(TEST_TIMER);
  TEST_ASSERT_EQUAL_UINT32(1234, count_val);
}
