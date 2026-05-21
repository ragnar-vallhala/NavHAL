#ifndef TEST_TIMER_H
#define TEST_TIMER_H

#include "navtest/navtest.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
// -------------------- Timer Init --------------------
void test_timer_init_sets_prescaler_and_arr(void);

// -------------------- Timer Start / Stop --------------------
void test_timer_start_sets_CEN_bit(void);
void test_timer_stop_clears_CEN_bit(void);

// -------------------- Timer Reset --------------------
void test_timer_reset_clears_count(void);

// -------------------- Compare / CCR --------------------
void test_timer_set_compare_and_get_compare(void);

// -------------------- Channel Enable / Disable --------------------
void test_timer_enable_and_disable_channel(void);

// -------------------- Timer Interrupt --------------------
void test_timer_enable_interrupt_sets_DIER_UIE(void);
void test_timer_clear_interrupt_flag_clears_UIF(void);

// -------------------- ARR Set / Get --------------------
void test_timer_set_arr_and_get_arr(void);

// -------------------- Timer Counter --------------------
void test_timer_get_count_returns_correct_value(void);

// -------------------- Frequency Calculation --------------------
void test_timer_get_frequency_returns_correct_value(void);

// -------------------- Additional Coverage --------------------
void test_hal_timer_init_rejects_null_config(void);
void test_hal_timer_init_freq_returns_ok(void);
void test_hal_timer_set_prescaler_round_trip(void);
void test_hal_timer_set_get_auto_reload(void);
void test_hal_timer_attach_then_detach_callback(void);

extern const navtest_suite_t test_timer_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_TIMER_H
