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
 * @file tests/test_interrupt.h
 * @brief Standardized hal_interrupt_* (NVIC) API tests.
 */

#ifndef TEST_INTERRUPT_H
#define TEST_INTERRUPT_H

#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
void test_hal_interrupt_enable_sets_iser_bit(void);
void test_hal_interrupt_disable_sets_icer_bit(void);
void test_hal_interrupt_clear_pending_clears_ispr_bit(void);
void test_hal_interrupt_set_get_priority_round_trip(void);
void test_hal_interrupt_is_pending_after_set(void);
void test_hal_interrupt_attach_then_dispatch_runs_callback(void);
void test_hal_interrupt_detach_clears_callback(void);
void test_hal_interrupt_disable_then_restore_global(void);
void test_hal_cpu_idle_returns_on_pending_irq(void);
void test_hal_interrupt_clear_all_pending_zeros_icpr(void);

/* error paths */
void test_hal_interrupt_enable_rejects_negative_irq(void);
void test_hal_interrupt_disable_rejects_negative_irq(void);
void test_hal_interrupt_clear_pending_rejects_negative_irq(void);
void test_hal_interrupt_attach_rejects_out_of_range(void);
void test_hal_interrupt_detach_rejects_out_of_range(void);

extern const navtest_suite_t test_interrupt_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_INTERRUPT_H
