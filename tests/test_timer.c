#include "core/cortex-m4/clock.h"
#include "core/cortex-m4/timer.h"
#include "core/cortex-m4/timer_reg.h"
#include "unity.h"
#include <stdint.h>

#define TEST_TIMER TIM2
#define TEST_PSC 83
#define TEST_ARR 999
#define TEST_CHANNEL 1
#define TEST_COMPARE_VALUE 500

// -------------------- Timer Init --------------------
void test_timer_init_sets_prescaler_and_arr(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  timer_init(TEST_TIMER, TEST_PSC, TEST_ARR);

  TEST_ASSERT_EQUAL_UINT32(TEST_PSC, timer->PSC);
  TEST_ASSERT_EQUAL_UINT32(TEST_ARR, timer->ARR);
}

// -------------------- Timer Start / Stop --------------------
void test_timer_start_sets_CEN_bit(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  timer_start(TEST_TIMER);
  TEST_ASSERT_BITS_HIGH(TIMx_CR1_CEN, timer->CR1);
}

void test_timer_stop_clears_CEN_bit(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  timer_stop(TEST_TIMER);
  TEST_ASSERT_BITS_LOW(TIMx_CR1_CEN, timer->CR1);
}

// -------------------- Timer Reset --------------------
void test_timer_reset_clears_count(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  timer->CNT = 1234;
  timer_reset(TEST_TIMER);
  TEST_ASSERT_EQUAL_UINT32(0, timer->CNT);
}

// -------------------- Compare / CCR --------------------
void test_timer_set_compare_and_get_compare(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  timer_set_compare(TEST_TIMER, TEST_CHANNEL, TEST_COMPARE_VALUE);

  // Check CCR register directly
  uint32_t ccr_val = 0;
  switch (TEST_CHANNEL) {
  case 1:
    ccr_val = timer->CCR1;
    break;
  case 2:
    ccr_val = timer->CCR2;
    break;
  case 3:
    ccr_val = timer->CCR3;
    break;
  case 4:
    ccr_val = timer->CCR4;
    break;
  }
  TEST_ASSERT_EQUAL_UINT32(TEST_COMPARE_VALUE, ccr_val);

  // Check timer_get_compare returns same value
  uint32_t compare_val = timer_get_compare(TEST_TIMER, TEST_CHANNEL);
  TEST_ASSERT_EQUAL_UINT32(TEST_COMPARE_VALUE, compare_val);
}

// -------------------- Channel Enable / Disable --------------------
void test_timer_enable_and_disable_channel(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  timer_disable_channel(TEST_TIMER, TEST_CHANNEL);
  TEST_ASSERT_BITS_LOW(TIMx_CCER_CCxE_MASK(TEST_CHANNEL), timer->CCER);

  timer_enable_channel(TEST_TIMER, TEST_CHANNEL);
  TEST_ASSERT_BITS_HIGH(TIMx_CCER_CCxE_MASK(TEST_CHANNEL), timer->CCER);
}

// -------------------- Timer Interrupt --------------------
void test_timer_enable_interrupt_sets_DIER_UIE(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  timer_enable_interrupt(TEST_TIMER);
  TEST_ASSERT_BITS_HIGH(TIMx_DIER_UIE, timer->DIER);
}

void test_timer_clear_interrupt_flag_clears_UIF(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  // Manually set UIF bit
  timer->SR |= TIMx_SR_UIF;
  TEST_ASSERT_BITS_HIGH(TIMx_SR_UIF, timer->SR);

  // Clear flag
  timer_clear_interrupt_flag(TEST_TIMER);

  // Correct check: UIF should now be 0
  TEST_ASSERT_BITS_LOW(TIMx_SR_UIF, timer->SR);
}

// -------------------- ARR Set / Get --------------------
void test_timer_set_arr_and_get_arr(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  timer_set_arr(TEST_TIMER, TEST_CHANNEL, TEST_ARR);
  uint32_t arr_val = timer_get_arr(TEST_TIMER, TEST_CHANNEL);

  TEST_ASSERT_EQUAL_UINT32(TEST_ARR, arr_val);
}

// -------------------- Timer Counter --------------------
void test_timer_get_count_returns_correct_value(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  timer_stop(TEST_TIMER);
  timer->CNT = 1234;

  uint32_t count_val = timer_get_count(TEST_TIMER);
  TEST_ASSERT_EQUAL_UINT32(1234, count_val);
}

// -------------------- Tick Tests --------------------
void test_systick_tick_increments(void) {
  uint64_t tick_before = (uint32_t)hal_get_tick();
  SysTick_Handler(); // manually call
  uint64_t tick_after = (uint32_t)hal_get_tick();
  TEST_ASSERT_EQUAL_UINT32(tick_before + 1, tick_after);
}

// -------------------- Frequency Calculation --------------------
void test_timer_get_frequency_returns_correct_value(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  timer_init(TEST_TIMER, 9, 99); // prescaler=9, ARR=99
  uint32_t freq = timer_get_frequency(TEST_TIMER);

  uint32_t timer_clk = hal_clock_get_apb1clk();
  uint32_t expected = timer_clk / (9 + 1) / (99 + 1);
  TEST_ASSERT_EQUAL_UINT32(expected, freq);
}
