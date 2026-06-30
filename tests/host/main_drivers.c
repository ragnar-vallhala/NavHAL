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
 * @file tests/host/main_drivers.c
 * @brief Entry point for the host *driver* suite — exercises the real vendor
 *        drivers against simulated MMIO (see host_mmio.h). Separate from
 *        main.c because this build links the STM32 driver sources and maps the
 *        peripheral region; the pure-logic host suite does neither.
 */

#include "host_mmio.h"
#include "navtest/navtest.h"

extern const navtest_suite_t test_gpio_driver_suite;
extern const navtest_suite_t test_uart_driver_suite;
extern const navtest_suite_t test_i2c_driver_suite;
extern const navtest_suite_t test_spi_driver_suite;
extern const navtest_suite_t test_clock_driver_suite;
extern const navtest_suite_t test_flash_driver_suite;

static const navtest_suite_t *const driver_suites[] = {
    &test_gpio_driver_suite,  &test_uart_driver_suite,
    &test_i2c_driver_suite,   &test_spi_driver_suite,
    &test_clock_driver_suite, &test_flash_driver_suite,
};

int main(void) {
  host_mmio_setup();

  navtest_write("\r\n"
                "|========================================|\r\n"
                "|     NAVHAL host-driver test suite      |\r\n"
                "|     (real drivers vs simulated MMIO)   |\r\n"
                "|========================================|\r\n");

  int failed = 0;
  uint32_t total = 0;
  const size_t n = sizeof(driver_suites) / sizeof(driver_suites[0]);
  for (size_t i = 0; i < n; i++) {
    failed += navtest_run_suite(driver_suites[i]);
    total += driver_suites[i]->count;
  }

  navtest_write("\n=========== FINAL RESULTS ===========\n");
  navtest_write("Total tests run: ");
  _navtest_print_uint32(total);
  navtest_write("\nTotal failures:  ");
  _navtest_print_uint32((uint32_t)failed);
  navtest_write("\n");
  return failed;
}
