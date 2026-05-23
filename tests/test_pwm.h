/**
 * @file test_pwm.h
 * @brief Unit tests for PWM driver (STM32F4 - Cortex-M4).
 */

#ifndef TEST_PWM_H
#define TEST_PWM_H

#include "navtest/navtest.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
// -------------------- Prototypes --------------------

// PWM Init
void test_hal_pwm_init_apb1(void);
void test_hal_pwm_init_apb2(void);

// PWM Start/Stop
void test_hal_pwm_start_sets_counter_enable(void);
void test_hal_pwm_stop_clears_counter_enable(void);

// PWM Duty Cycle
void test_hal_pwm_set_duty_cycle_updates_ccr(void);

// Additional standardized coverage
void test_hal_pwm_init_rejects_null_handle(void);
void test_hal_pwm_start_rejects_null_handle(void);
void test_hal_pwm_stop_rejects_null_handle(void);
void test_hal_pwm_set_duty_cycle_rejects_null_handle(void);
void test_hal_pwm_set_frequency_returns_ok(void);
void test_hal_pwm_set_frequency_rejects_null_handle(void);

extern const navtest_suite_t test_pwm_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_PWM_H

