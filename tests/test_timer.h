#ifndef TEST_TIMER_H
#define TEST_TIMER_H

#include "unity.h"
#include "core/cortex-m4/timer.h"
#include "navhal.h"
#include <stdint.h>

void test_timer_init_sets_prescaler_and_arr(void);
void test_timer_start_sets_CEN_bit(void);
void test_timer_stop_clears_CEN_bit(void);
void test_timer_reset_clears_count(void);
void test_timer_set_compare_and_get_compare(void);
void test_timer_enable_and_disable_channel(void);
void test_timer_enable_and_disable_interrupt(void);
void test_timer_clear_interrupt_flag_clears_UIF(void);
void test_timer_get_arr_returns_arr_value(void);
void test_timer_get_count_returns_count_value(void);


#endif // TEST_TIMER_H

