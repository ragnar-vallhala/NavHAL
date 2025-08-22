#ifndef TEST_TIMER_H
#define TEST_TIMER_H

#include <stdint.h>

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

// -------------------- Tick Tests --------------------
void test_systick_tick_increments(void);

// -------------------- Frequency Calculation --------------------
void test_timer_get_frequency_returns_correct_value(void);

#endif // TEST_TIMER_H
