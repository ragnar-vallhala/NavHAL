/*
 * Copyright (C) 2026 NAVRobotec Pvt Ltd
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

/** @file tests/arch/avr/test_interrupt.h @brief AVR interrupt-flag suite. */

#ifndef NAVTEST_ARCH_AVR_TEST_INTERRUPT_H
#define NAVTEST_ARCH_AVR_TEST_INTERRUPT_H

#include "navtest/navtest.h"

void test_avr_enable_sets_the_peripheral_mask_bit(void);

void test_avr_clear_pending_leaves_neighbours_standing(void);

extern const navtest_suite_t test_avr_interrupt_suite;

#endif /* NAVTEST_ARCH_AVR_TEST_INTERRUPT_H */
