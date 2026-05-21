#ifndef TEST_DWT_H
#define TEST_DWT_H

#include "common/hal_features.h"
#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
#if NAVHAL_HAS_CYCLE_COUNTER

void test_dwt_init_enables_counters(void);
void test_dwt_get_cycles_increments(void);
void test_dwt_reset_cycles_zeros_counter(void);
void test_dwt_delay_cycles_elapses_time(void);
void test_hal_cycle_counter_init_returns_ok(void);
void test_hal_cycle_counter_reset_returns_ok(void);

extern const navtest_suite_t test_dwt_suite;

#endif /* NAVHAL_HAS_CYCLE_COUNTER */

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_DWT_H
