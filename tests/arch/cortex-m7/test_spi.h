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

#ifndef TEST_SPI_H
#define TEST_SPI_H

#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
void test_spi_init_config(void);
void test_hal_spi_init_returns_ok(void);
void test_hal_spi_init_rejects_null_config(void);
void test_hal_spi_transmit_rejects_null_data(void);
void test_hal_spi_receive_rejects_null_data(void);
void test_hal_spi_transmit_receive_rejects_null_data(void);
void test_hal_spi_init_cpol_low_cpha_1edge(void);

extern const navtest_suite_t test_spi_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_SPI_H
