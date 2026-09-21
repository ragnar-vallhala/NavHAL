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
 * @file tests/test_gpio.h
 * @brief Standardized hal_gpio_* API tests.
 */

#ifndef TEST_GPIO_H
#define TEST_GPIO_H

#include "navtest/navtest.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
/* All tests target this pin (PC09 — unused on Nucleo-F401RE board header). */
#define TEST_GPIO_PIN GPIO_PC09
#define TEST_GPIO_AF  HAL_GPIO_AF7

void test_hal_gpio_init_applies_config(void);
void test_hal_gpio_init_rejects_null_config(void);
void test_hal_gpio_set_mode_writes_moder(void);
void test_hal_gpio_get_mode_round_trip(void);
void test_hal_gpio_enable_clock_sets_ahb1_bit(void);
void test_hal_gpio_set_alternate_function_writes_af(void);
void test_hal_gpio_set_alternate_function_switches_mode_to_af(void);
void test_hal_gpio_set_output_type_writes_otyper(void);
void test_hal_gpio_set_output_speed_writes_ospeedr(void);
void test_hal_gpio_write_high_then_low(void);
void test_hal_gpio_toggle_flips_state(void);
void test_hal_gpio_port_indexing_is_contiguous(void);

extern const navtest_suite_t test_gpio_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_GPIO_H
