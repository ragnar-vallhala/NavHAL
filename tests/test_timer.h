/**
 * @file test_timer.h
 * @brief Timer unit test declarations for NAVHAL
 * @details
 * This header defines the test cases for verifying Timer peripheral functionality
 * in the NAVHAL hardware abstraction layer. It includes tests for:
 * - Timer initialization and configuration
 * - Start/stop control
 * - Counter operations
 * - Compare register functionality
 * - Channel control
 * - Interrupt handling
 *
 * @note Uses Unity test framework for assertions
 * @note Tests cover both general-purpose and advanced timers
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef TEST_TIMER_H
#define TEST_TIMER_H
#include "unity.h"
#include <stdint.h>

/**
 * @brief Test timer initialization
 * @details Verifies timer_init() correctly sets prescaler and auto-reload values
 */
void test_timer_init_sets_prescaler_and_arr(void);

/**
 * @brief Test timer start functionality
 * @details Verifies timer_start() properly sets the counter enable (CEN) bit
 */
void test_timer_start_sets_CEN_bit(void);

/**
 * @brief Test timer stop functionality
 * @details Verifies timer_stop() properly clears the counter enable (CEN) bit
 */
void test_timer_stop_clears_CEN_bit(void);

/**
 * @brief Test timer counter reset
 * @details Verifies timer_reset() clears the counter value to zero
 */
void test_timer_reset_clears_count(void);

/**
 * @brief Test compare register operations
 * @details Verifies timer_set_compare() and timer_get_compare() functionality
 */
void test_timer_set_compare_and_get_compare(void);

/**
 * @brief Test channel enable/disable
 * @details Verifies timer_enable_channel() and timer_disable_channel() functions
 */
void test_timer_enable_and_disable_channel(void);

/**
 * @brief Test interrupt control
 * @details Verifies timer interrupt enable/disable functionality
 * @note May affect other tests if interrupts are globally enabled
 */
void test_timer_enable_and_disable_interrupt(void);

/**
 * @brief Test interrupt flag clearing
 * @details Verifies timer_clear_interrupt_flag() clears the update interrupt flag
 */
void test_timer_clear_interrupt_flag_clears_UIF(void);


/**
 * @brief Test auto-reload register access
 * @details Verifies timer_get_arr() returns the correct auto-reload value
 */
void test_timer_get_arr_returns_arr_value(void);

/**
 * @brief Test counter value access
 * @details Verifies timer_get_count() returns the current counter value
 */
void test_timer_get_count_returns_count_value(void);

#endif // TEST_TIMER_H

