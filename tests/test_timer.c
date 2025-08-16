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
  timer_init(TEST_TIMER, TEST_PSC, TEST_ARR);

  volatile uint32_t *psc = (volatile uint32_t *)(TIM2_BASE + TIM_GP1_PSC_OFFSET);
  volatile uint32_t *arr = (volatile uint32_t *)(TIM2_BASE + TIM_GP1_ARR_OFFSET);

  TEST_ASSERT_EQUAL_UINT32(TEST_PSC, *psc);
  TEST_ASSERT_EQUAL_UINT32(TEST_ARR, *arr);
}

/**
 * @brief Test timer start functionality
 * @details
 * Verifies that timer_start() sets the Counter ENable (CEN) bit
 * in the CR1 register
 */
void test_timer_start_sets_CEN_bit(void) {
  timer_start(TEST_TIMER);

  volatile uint32_t *cr1 = (volatile uint32_t *)(TIM2_BASE + TIM_GP1_CR1_OFFSET);
  TEST_ASSERT_BITS_HIGH(1 << TIM_GP1_CR1_CEN_BIT, *cr1);
}

/**
 * @brief Test timer stop functionality
 * @details
 * Verifies that timer_stop() clears the Counter ENable (CEN) bit
 * in the CR1 register
 */
void test_timer_stop_clears_CEN_bit(void) {
  timer_stop(TEST_TIMER);

  volatile uint32_t *cr1 = (volatile uint32_t *)(TIM2_BASE + TIM_GP1_CR1_OFFSET);
  TEST_ASSERT_BITS_LOW(1 << TIM_GP1_CR1_CEN_BIT, *cr1);
}

/**
 * @brief Test timer counter reset
 * @details
 * Verifies that timer_reset() clears the counter value to zero
 * by checking the CNT register
 */
void test_timer_reset_clears_count(void) {
  // Set count to a non-zero value
  volatile uint32_t *cnt = (volatile uint32_t *)(TIM2_BASE + TIM_GP1_CNT_OFFSET);
  *cnt = 12345;

  timer_reset(TEST_TIMER);

  TEST_ASSERT_EQUAL_UINT32(0, *cnt);
}

/**
 * @brief Test compare register operations
 * @details
 * Verifies:
 * 1. timer_set_compare() correctly sets the compare value in CCR register
 * 2. timer_get_compare() correctly returns the compare value
 */
void test_timer_set_compare_and_get_compare(void) {
  timer_set_compare(TEST_TIMER, TEST_CHANNEL, TEST_COMPARE_VALUE);

  volatile uint32_t *ccr = (volatile uint32_t *)(TIM2_BASE + TIMx_CCR1_OFFSET);
  TEST_ASSERT_EQUAL_UINT32(TEST_COMPARE_VALUE, *ccr);

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
  timer_disable_channel(TEST_TIMER, TEST_CHANNEL);

  volatile uint32_t *ccer = (volatile uint32_t *)(TIM2_BASE + TIMx_CCER_OFFSET);
  TEST_ASSERT_BITS_LOW(1 << TIMx_CCER_CC1E_BIT, *ccer);

  timer_enable_channel(TEST_TIMER, TEST_CHANNEL);
  TEST_ASSERT_BITS_HIGH(1 << TIMx_CCER_CC1E_BIT, *ccer);
}

/**
 * @brief Test interrupt enable
 * @details
 * Verifies that timer_enable_interrupt() sets the Update Interrupt Enable (UIE) bit
 * in the DIER register
 */
void test_timer_enable_and_disable_interrupt(void) {
  volatile uint32_t *dier = (volatile uint32_t *)(TIM2_BASE + TIM_GP1_DIER_OFFSET);

  timer_enable_interrupt(TEST_TIMER);
  TEST_ASSERT_BITS_HIGH(1 << TIM_GP1_DIER_UIE_BIT, *dier);
}

/**
 * @brief Test interrupt flag clearing
 * @details
 * Verifies that timer_clear_interrupt_flag() clears the Update Interrupt Flag (UIF)
 * in the SR register
 */
void test_timer_clear_interrupt_flag_clears_UIF(void) {
  volatile uint32_t *sr = (volatile uint32_t *)(TIM2_BASE + TIM_GP1_SR_OFFSET);
  *sr |= (1 << TIM_GP1_SR_UIF_BIT);

  timer_clear_interrupt_flag(TEST_TIMER);
  TEST_ASSERT_BITS_LOW(1 << TIM_GP1_SR_UIF_BIT, *sr);
}

/**
 * @brief Test auto-reload register access
 * @details
 * Verifies that timer_get_arr() correctly returns the value
 * from the ARR register
 */
void test_timer_get_arr_returns_arr_value(void) {
  volatile uint32_t *arr = (volatile uint32_t *)(TIM2_BASE + TIM_GP1_ARR_OFFSET);
  *arr = TEST_ARR;

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
  volatile uint32_t *cnt = (volatile uint32_t *)(TIM2_BASE + TIM_GP1_CNT_OFFSET);
  *cnt = 1234;

  uint32_t count_val = timer_get_count(TEST_TIMER);
  TEST_ASSERT_EQUAL_UINT32(1234, count_val);
}