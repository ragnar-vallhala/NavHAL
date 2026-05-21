#include "navhal_port_clock.h"
#include "navhal_port_timer.h"
#include "family/timer_reg.h"
#include "family/rcc_reg.h"
#include "navtest/navtest.h"
#include "navtest/navtest_pil.h"
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

  hal_timer_config_t cfg = {.prescaler = TEST_PSC, .auto_reload = TEST_ARR};
  hal_timer_init(TEST_TIMER, &cfg);

  TEST_ASSERT_EQUAL_UINT32(TEST_PSC, timer->PSC);
  TEST_ASSERT_EQUAL_UINT32(TEST_ARR, timer->ARR);
}

// -------------------- Timer Start / Stop --------------------
void test_timer_start_sets_CEN_bit(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  hal_timer_start(TEST_TIMER);
  TEST_ASSERT_BITS_HIGH(TIMx_CR1_CEN, timer->CR1);
}

void test_timer_stop_clears_CEN_bit(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  hal_timer_stop(TEST_TIMER);
  TEST_ASSERT_BITS_LOW(TIMx_CR1_CEN, timer->CR1);
}

// -------------------- Timer Reset --------------------
void test_timer_reset_clears_count(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  timer->CNT = 1234;
  hal_timer_reset(TEST_TIMER);
  TEST_ASSERT_EQUAL_UINT32(0, timer->CNT);
}

// -------------------- Compare / CCR --------------------
void test_timer_set_compare_and_get_compare(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  hal_timer_set_compare(TEST_TIMER, TEST_CHANNEL, TEST_COMPARE_VALUE);

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

  // Check hal_timer_get_compare returns same value
  uint32_t compare_val = hal_timer_get_compare(TEST_TIMER, TEST_CHANNEL);
  TEST_ASSERT_EQUAL_UINT32(TEST_COMPARE_VALUE, compare_val);
}

// -------------------- Channel Enable / Disable --------------------
void test_timer_enable_and_disable_channel(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  hal_timer_disable_channel(TEST_TIMER, TEST_CHANNEL);
  TEST_ASSERT_BITS_LOW(TIMx_CCER_CCxE_MASK(TEST_CHANNEL), timer->CCER);

  hal_timer_enable_channel(TEST_TIMER, TEST_CHANNEL);
  TEST_ASSERT_BITS_HIGH(TIMx_CCER_CCxE_MASK(TEST_CHANNEL), timer->CCER);
}

// -------------------- Timer Interrupt --------------------
void test_timer_enable_interrupt_sets_DIER_UIE(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  hal_timer_enable_interrupt(TEST_TIMER);
  TEST_ASSERT_BITS_HIGH(TIMx_DIER_UIE, timer->DIER);
}

void test_timer_clear_interrupt_flag_clears_UIF(void) {
  NAVTEST_SKIP_ON_PIL();
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  // Manually set UIF bit
  timer->SR |= TIMx_SR_UIF;
  TEST_ASSERT_BITS_HIGH(TIMx_SR_UIF, timer->SR);

  // Clear flag
  hal_timer_clear_interrupt_flag(TEST_TIMER);

  // Correct check: UIF should now be 0
  TEST_ASSERT_BITS_LOW(TIMx_SR_UIF, timer->SR);
}

// -------------------- ARR Set / Get --------------------
void test_timer_set_arr_and_get_arr(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  hal_timer_set_auto_reload(TEST_TIMER, TEST_ARR);
  uint32_t arr_val = hal_timer_get_auto_reload(TEST_TIMER);

  TEST_ASSERT_EQUAL_UINT32(TEST_ARR, arr_val);
}

// -------------------- Timer Counter --------------------
void test_timer_get_count_returns_correct_value(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  hal_timer_stop(TEST_TIMER);
  timer->CNT = 1234;

  uint32_t count_val = hal_timer_get_count(TEST_TIMER);
  TEST_ASSERT_EQUAL_UINT32(1234, count_val);
}

// -------------------- Tick Tests --------------------
// -------------------- Frequency Calculation --------------------
void test_timer_get_frequency_returns_correct_value(void) {
  TIMx_Reg_Typedef *timer = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_NOT_NULL(timer);

  hal_timer_config_t cfg = {.prescaler = 9, .auto_reload = 99};
  hal_timer_init(TEST_TIMER, &cfg); // prescaler=9, ARR=99
  uint32_t freq = hal_timer_get_frequency(TEST_TIMER);

  uint32_t timer_clk;
  uint32_t ppre1 = (RCC->CFGR >> RCC_CFGR_PPRE1_BIT) & 0x7;
  uint32_t apb1 = hal_clock_get_apb1clk();
  timer_clk = (ppre1 == 0) ? apb1 : (apb1 * 2);

  uint32_t expected = timer_clk / (9 + 1) / (99 + 1);
  TEST_ASSERT_EQUAL_UINT32(expected, freq);
}

/* -------------------- Additional standardized-API coverage -------------------- */

void test_hal_timer_init_rejects_null_config(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_timer_init(TEST_TIMER, NULL));
}

void test_hal_timer_init_freq_returns_ok(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_timer_init_freq(TEST_TIMER, 1000));
}

void test_hal_timer_set_prescaler_round_trip(void) {
  hal_timer_config_t cfg = {.prescaler = TEST_PSC, .auto_reload = TEST_ARR};
  hal_timer_init(TEST_TIMER, &cfg);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_timer_set_prescaler(TEST_TIMER, 42));
  TIMx_Reg_Typedef *t = GET_TIMx_BASE(TEST_TIMER);
  TEST_ASSERT_EQUAL_UINT32(42u, t->PSC & 0xFFFFu);
}

void test_hal_timer_set_get_auto_reload(void) {
  hal_timer_config_t cfg = {.prescaler = TEST_PSC, .auto_reload = TEST_ARR};
  hal_timer_init(TEST_TIMER, &cfg);
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_OK,
      (uint32_t)hal_timer_set_auto_reload(TEST_TIMER, 1234));
  TEST_ASSERT_EQUAL_UINT32(1234u,
                           hal_timer_get_auto_reload(TEST_TIMER));
}

static volatile uint32_t s_timer_cb_hits = 0;
static void timer_test_cb(void) { s_timer_cb_hits++; }

void test_hal_timer_attach_then_detach_callback(void) {
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_OK,
      (uint32_t)hal_timer_attach_callback(TEST_TIMER, timer_test_cb));
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_OK, (uint32_t)hal_timer_detach_callback(TEST_TIMER));
}

static const navtest_case_t timer_cases[] = {
    NAVTEST_CASE(test_timer_init_sets_prescaler_and_arr),
    NAVTEST_CASE(test_timer_start_sets_CEN_bit),
    NAVTEST_CASE(test_timer_stop_clears_CEN_bit),
    NAVTEST_CASE(test_timer_reset_clears_count),
    NAVTEST_CASE(test_timer_set_compare_and_get_compare),
    NAVTEST_CASE(test_timer_enable_and_disable_channel),
    /* test_timer_enable_and_disable_interrupt — disabled: hangs UART */
    NAVTEST_CASE(test_timer_clear_interrupt_flag_clears_UIF),
    NAVTEST_CASE(test_timer_set_arr_and_get_arr),
    NAVTEST_CASE(test_timer_get_count_returns_correct_value),
    NAVTEST_CASE(test_timer_get_frequency_returns_correct_value),
    /* extra coverage */
    NAVTEST_CASE(test_hal_timer_init_rejects_null_config),
    NAVTEST_CASE(test_hal_timer_init_freq_returns_ok),
    NAVTEST_CASE(test_hal_timer_set_prescaler_round_trip),
    NAVTEST_CASE(test_hal_timer_set_get_auto_reload),
    NAVTEST_CASE(test_hal_timer_attach_then_detach_callback),
};

const navtest_suite_t test_timer_suite = {
    .name = "TIMER",
    .cases = timer_cases,
    .count = sizeof(timer_cases) / sizeof(timer_cases[0]),
    .between = NULL,
};
