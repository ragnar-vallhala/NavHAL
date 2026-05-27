/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

