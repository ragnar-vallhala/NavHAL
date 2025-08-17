/**
 * @file test_pwm.h
 * @brief Unit tests for PWM driver (STM32F4 - Cortex-M4).
 */

#ifndef TEST_PWM_H
#define TEST_PWM_H

#include <stdint.h>

// -------------------- Prototypes --------------------

// PWM Init
void test_hal_pwm_init_apb1(void);
void test_hal_pwm_init_apb2(void);

// PWM Start/Stop
void test_hal_pwm_start_sets_counter_enable(void);
void test_hal_pwm_stop_clears_counter_enable(void);

// PWM Duty Cycle
void test_hal_pwm_set_duty_cycle_updates_ccr(void);

#endif // TEST_PWM_H

