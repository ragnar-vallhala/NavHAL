#ifndef TEST_DWT_H
#define TEST_DWT_H

#include "navtest/navtest.h"

void test_dwt_init_enables_counters(void);
void test_dwt_get_cycles_increments(void);
void test_dwt_reset_cycles_zeros_counter(void);
void test_dwt_delay_cycles_elapses_time(void);

extern const navtest_suite_t test_dwt_suite;

#endif // TEST_DWT_H
