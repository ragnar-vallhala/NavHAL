/**
 * @file tests/test_timebase.h
 * @brief Standardized hal_timebase_* API tests.
 *
 * The timebase replaces the SysTick-specific naming from M2. The
 * timebase concept generalizes to non-Cortex MCUs (M6 / AVR will use a
 * general-purpose timer for the same role).
 */

#ifndef TEST_TIMEBASE_H
#define TEST_TIMEBASE_H

#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
void test_hal_timebase_init_returns_ok(void);
void test_hal_timebase_tick_increments(void);
void test_hal_timebase_get_tick_duration_us_matches_init(void);
void test_hal_timebase_get_millis_is_monotonic(void);
void test_hal_timebase_get_micros_is_monotonic(void);
void test_hal_timebase_get_reload_value_nonzero(void);
void test_hal_timebase_set_callback_rejects_null(void);
void test_hal_timebase_returns_uint32(void);

extern const navtest_suite_t test_timebase_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_TIMEBASE_H
