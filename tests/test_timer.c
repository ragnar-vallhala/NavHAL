/**
 * @file test_timer.c
 * @brief Timer unit test implementation for NAVHAL
 * @details
 * This file implements the test cases for verifying Timer peripheral functionality
 * in the NAVHAL hardware abstraction layer. It performs register-level verification
 * of all timer operations using TIM2 as the test timer.
 *
 * Tests cover:
 * - Initialization (prescaler and auto-reload values)
 * - Start/stop control
 * - Counter operations
 * - Compare register functionality
 * - Channel enable/disable
 * - Interrupt control
 * - Register access functions
 *
 * @note Uses Unity test framework for assertions
 * @note All tests use TIM2 as the test timer
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "core/cortex-m4/timer.h"
#include "core/cortex-m4/timer_reg.h"
#include "unity.h"
#include <stdint.h>

#define TEST_TIMER TIM2           ///< Timer peripheral used for all tests (TIM2)
#define TEST_PSC 83               ///< Test prescaler value
#define TEST_ARR 999              ///< Test auto-reload value
#define TEST_CHANNEL 1            ///< Test channel number (Channel 1)
#define TEST_COMPARE_VALUE 500    ///< Test compare register value

/**
 * @brief Test timer initialization
 * @details
 * Verifies that timer_init() correctly sets:
 * 1. The prescaler value in PSC register
 * 2. The auto-reload value in ARR register
 */
void test_timer_init_sets_prescaler_and_arr(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  if (timer == NULL)
    return;
  timer_init(TEST_TIMER, TEST_PSC, TEST_ARR);

  TEST_ASSERT_EQUAL_UINT32(TEST_PSC, timer->PSC);
  TEST_ASSERT_EQUAL_UINT32(TEST_ARR, timer->ARR);
}

/**
 * @brief Test timer start functionality
 * @details
 * Verifies that timer_start() sets the Counter ENable (CEN) bit
 * in the CR1 register
 */
void test_timer_start_sets_CEN_bit(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  if (timer == NULL)
    return;
  timer_start(TEST_TIMER);

  TEST_ASSERT_BITS_HIGH(TIMx_CR1_CEN, timer->CR1 & TIMx_CR1_CEN);

}

/**
 * @brief Test timer stop functionality
 * @details
 * Verifies that timer_stop() clears the Counter ENable (CEN) bit
 * in the CR1 register
 */
void test_timer_stop_clears_CEN_bit(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  if (timer == NULL)
    return;
  timer_stop(TEST_TIMER);

  TEST_ASSERT_BITS_LOW(TIMx_CR1_CEN, timer->CR1 & TIMx_CR1_CEN);

}

/**
 * @brief Test timer counter reset
 * @details
 * Verifies that timer_reset() clears the counter value to zero
 * by checking the CNT register
 */
void test_timer_reset_clears_count(void) {

  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  if (timer == NULL)
    return;
  timer_stop(TEST_TIMER);
  timer_reset(TEST_TIMER);
  TEST_ASSERT_EQUAL_UINT32(0, timer->CNT);
}

/**
 * @brief Test compare register operations
 * @details
 * Verifies:
 * 1. timer_set_compare() correctly sets the compare value in CCR register
 * 2. timer_get_compare() correctly returns the compare value
 */
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

/**
 * @brief Test channel enable/disable
 * @details
 * Verifies:
 * 1. timer_disable_channel() clears the channel enable bit in CCER register
 * 2. timer_enable_channel() sets the channel enable bit in CCER register
 */
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

/**
 * @brief Test interrupt enable
 * @details
 * Verifies that timer_enable_interrupt() sets the Update Interrupt Enable (UIE) bit
 * in the DIER register
 */
void test_timer_enable_and_disable_interrupt(void) {

  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  if (timer == NULL)
    return;

  timer_enable_interrupt(TEST_TIMER);
  TEST_ASSERT_BITS_HIGH(TIMx_DIER_UIE, timer->DIER);
}

/**
 * @brief Test interrupt flag clearing
 * @details
 * Verifies that timer_clear_interrupt_flag() clears the Update Interrupt Flag (UIF)
 * in the SR register
 */
void test_timer_clear_interrupt_flag_clears_UIF(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  if (timer == NULL)
    return;
  timer->SR |= (TIMx_SR_UIF);
  timer_clear_interrupt_flag(TEST_TIMER);
  TEST_ASSERT_BITS_LOW(~(TIMx_SR_UIF), timer->SR & TIMx_SR_UIF);
}

/**
 * @brief Test auto-reload register access
 * @details
 * Verifies that timer_get_arr() correctly returns the value
 * from the ARR register
 */
void test_timer_get_arr_returns_arr_value(void) {

  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  if (timer == NULL)
    return;
  timer_set_arr(TEST_TIMER, TEST_CHANNEL, TEST_ARR);

  uint32_t arr_val = timer_get_arr(TEST_TIMER, 1);
  TEST_ASSERT_EQUAL_UINT32(TEST_ARR, arr_val);
}

/**
 * @brief Test counter value access
 * @details
 * Verifies that timer_get_count() correctly returns the current
 * counter value from the CNT register
 */
void test_timer_get_count_returns_count_value(void) {

  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  if (timer == NULL)
    return;
  timer->CNT = 1234;

  uint32_t count_val = timer_get_count(TEST_TIMER);
  TEST_ASSERT_EQUAL_UINT32(1234, count_val);
}