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
 * @file test_gpio_driver.c
 * @brief Deep host (SIL) tests for the real gpio.c against simulated MMIO.
 *
 * Exercises the actual driver register manipulation (MODER/OTYPER/OSPEEDR/
 * PUPDR/AFRL/AFRH/ODR/BSRR/IDR + RCC.AHB1ENR) on every port and pin, plus the
 * F7 contiguous-port indexing. gpio.c has no busy-waits, so it runs verbatim.
 */

#include "host_mmio.h"
#include "navhal_port_gpio.h"
#include "family/gpio_reg.h"
#include "family/rcc_reg.h"
#include "navtest/navtest.h"
#include <stdint.h>

static volatile GPIOx_Typedef *port_of(hal_gpio_pin_t pin) {
  return (volatile GPIOx_Typedef *)GPIO_GET_PORT(pin);
}
static uint32_t pin_of(hal_gpio_pin_t pin) { return GPIO_GET_PIN(pin); }

static uint32_t field2(uint32_t reg, uint32_t pin) {
  return (reg >> (pin * 2)) & 0x3u;
}

/* -------------------- contiguous port indexing (F7) -------------------- */

void test_host_gpio_port_indexing(void) {
  TEST_ASSERT_EQUAL_UINT32(0u, (uint32_t)GPIO_GET_PORT_NUMBER(GPIO_PA00));
  TEST_ASSERT_EQUAL_UINT32(1u, (uint32_t)GPIO_GET_PORT_NUMBER(GPIO_PB00));
  TEST_ASSERT_EQUAL_UINT32(5u, (uint32_t)GPIO_GET_PORT_NUMBER(GPIO_PF00));
  TEST_ASSERT_EQUAL_UINT32(6u, (uint32_t)GPIO_GET_PORT_NUMBER(GPIO_PG00));
  TEST_ASSERT_EQUAL_UINT32(7u, (uint32_t)GPIO_GET_PORT_NUMBER(GPIO_PH00));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)GPIOF_BASE_ADDR,
                           (uint32_t)(uintptr_t)GPIO_GET_PORT(GPIO_PF03));
  TEST_ASSERT_EQUAL_UINT32(3u, pin_of(GPIO_PF03));
}

/* -------------------- enable_clock sweeps all ports -------------------- */

void test_host_gpio_enable_clock_all_ports(void) {
  host_mmio_reset();
  /* One representative pin per port group; AHB1ENR bit == port number. */
  const hal_gpio_pin_t first[] = {GPIO_PA00, GPIO_PB00, GPIO_PC00, GPIO_PD00,
                                  GPIO_PE00, GPIO_PF00, GPIO_PG00, GPIO_PH00};
  for (uint32_t p = 0; p < 8; p++) {
    TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                             (uint32_t)hal_gpio_enable_clock(first[p]));
    TEST_ASSERT_BITS_HIGH((1u << p), RCC->AHB1ENR);
  }
  /* All eight bits now set, nothing above. */
  TEST_ASSERT_EQUAL_UINT32(0xFFu, RCC->AHB1ENR & 0xFFu);
}

/* -------------------- set_mode: every mode, every pin -------------------- */

void test_host_gpio_set_mode_all_pins_all_modes(void) {
  const hal_gpio_mode_t modes[] = {HAL_GPIO_MODE_INPUT, HAL_GPIO_MODE_OUTPUT,
                                   HAL_GPIO_MODE_AF, HAL_GPIO_MODE_ANALOG};
  for (uint32_t pin = 0; pin < 16; pin++) {
    for (uint32_t m = 0; m < 4; m++) {
      host_mmio_reset();
      hal_gpio_pin_t gpin = (hal_gpio_pin_t)(GPIO_PB00 + pin); /* port B */
      TEST_ASSERT_EQUAL_UINT32(
          (uint32_t)HAL_OK,
          (uint32_t)hal_gpio_set_mode(gpin, modes[m], HAL_GPIO_PULL_NONE));
      /* Only this pin's 2-bit MODER field is set; the rest stay 0. */
      TEST_ASSERT_EQUAL_UINT32(modes[m], field2(port_of(gpin)->MODER, pin));
      uint32_t others = port_of(gpin)->MODER & ~(0x3u << (pin * 2));
      TEST_ASSERT_EQUAL_UINT32(0u, others);
      /* set_mode also enables the port clock (B = AHB1ENR bit 1). */
      TEST_ASSERT_BITS_HIGH((1u << 1), RCC->AHB1ENR);
    }
  }
}

void test_host_gpio_set_mode_writes_pupdr(void) {
  const hal_gpio_pull_t pulls[] = {HAL_GPIO_PULL_NONE, HAL_GPIO_PULL_UP,
                                   HAL_GPIO_PULL_DOWN};
  for (uint32_t k = 0; k < 3; k++) {
    host_mmio_reset();
    hal_gpio_set_mode(GPIO_PC07, HAL_GPIO_MODE_INPUT, pulls[k]);
    TEST_ASSERT_EQUAL_UINT32(pulls[k], field2(port_of(GPIO_PC07)->PUPDR, 7));
  }
}

void test_host_gpio_get_mode_round_trip(void) {
  host_mmio_reset();
  hal_gpio_set_mode(GPIO_PD12, HAL_GPIO_MODE_ANALOG, HAL_GPIO_PULL_NONE);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_GPIO_MODE_ANALOG,
                           (uint32_t)hal_gpio_get_mode(GPIO_PD12));
  hal_gpio_set_mode(GPIO_PD12, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_GPIO_MODE_OUTPUT,
                           (uint32_t)hal_gpio_get_mode(GPIO_PD12));
}

/* -------------------- alternate function: nibble + AFRL/AFRH split ------- */

void test_host_gpio_alternate_function_all_pins(void) {
  for (uint32_t pin = 0; pin < 16; pin++) {
    host_mmio_reset();
    hal_gpio_pin_t gpin = (hal_gpio_pin_t)(GPIO_PE00 + pin);
    hal_gpio_af_t af = (hal_gpio_af_t)(pin % 16);
    TEST_ASSERT_EQUAL_UINT32(
        (uint32_t)HAL_OK,
        (uint32_t)hal_gpio_set_alternate_function(gpin, af));
    /* Mode switched to AF. */
    TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_GPIO_MODE_AF,
                             field2(port_of(gpin)->MODER, pin));
    /* AF nibble lands in AFRL (pins 0–7) or AFRH (8–15). */
    uint32_t reg = (pin < 8) ? port_of(gpin)->AFRL : port_of(gpin)->AFRH;
    uint32_t nib = (reg >> (4 * (pin % 8))) & 0xFu;
    TEST_ASSERT_EQUAL_UINT32((uint32_t)af, nib);
  }
}

/* -------------------- output type / speed -------------------- */

void test_host_gpio_output_type(void) {
  host_mmio_reset();
  hal_gpio_set_output_type(GPIO_PB10, HAL_GPIO_OTYPE_OPEN_DRAIN);
  TEST_ASSERT_BITS_HIGH((1u << 10), port_of(GPIO_PB10)->OTYPER);
  hal_gpio_set_output_type(GPIO_PB10, HAL_GPIO_OTYPE_PUSH_PULL);
  TEST_ASSERT_BITS_LOW((1u << 10), port_of(GPIO_PB10)->OTYPER);
}

void test_host_gpio_output_speed(void) {
  host_mmio_reset();
  hal_gpio_set_output_speed(GPIO_PA09, HAL_GPIO_SPEED_VERY_HIGH);
  TEST_ASSERT_EQUAL_UINT32(3u, field2(port_of(GPIO_PA09)->OSPEEDR, 9));
}

/* -------------------- write / read / toggle -------------------- */

void test_host_gpio_write_uses_bsrr(void) {
  host_mmio_reset();
  hal_gpio_write(GPIO_PG05, HAL_GPIO_HIGH);
  TEST_ASSERT_EQUAL_UINT32((1u << 5), port_of(GPIO_PG05)->BSRR);
  host_mmio_reset();
  hal_gpio_write(GPIO_PG05, HAL_GPIO_LOW);
  TEST_ASSERT_EQUAL_UINT32((1u << (5 + 16)), port_of(GPIO_PG05)->BSRR);
}

void test_host_gpio_read_uses_idr(void) {
  host_mmio_reset();
  port_of(GPIO_PC13)->IDR = (1u << 13);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_GPIO_HIGH,
                           (uint32_t)hal_gpio_read(GPIO_PC13));
  port_of(GPIO_PC13)->IDR = 0;
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_GPIO_LOW,
                           (uint32_t)hal_gpio_read(GPIO_PC13));
}

void test_host_gpio_toggle_flips_odr(void) {
  host_mmio_reset();
  hal_gpio_toggle(GPIO_PB00);
  TEST_ASSERT_EQUAL_UINT32(1u, port_of(GPIO_PB00)->ODR & 1u);
  hal_gpio_toggle(GPIO_PB00);
  TEST_ASSERT_EQUAL_UINT32(0u, port_of(GPIO_PB00)->ODR & 1u);
}

/* -------------------- init applies full config -------------------- */

void test_host_gpio_init_applies_config(void) {
  host_mmio_reset();
  hal_gpio_config_t cfg = {
      .mode = HAL_GPIO_MODE_AF,
      .pull = HAL_GPIO_PULL_UP,
      .output_type = HAL_GPIO_OTYPE_OPEN_DRAIN,
      .output_speed = HAL_GPIO_SPEED_HIGH,
      .alternate = HAL_GPIO_AF7,
  };
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_gpio_init(GPIO_PB09, &cfg));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_GPIO_MODE_AF,
                           field2(port_of(GPIO_PB09)->MODER, 9));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_GPIO_PULL_UP,
                           field2(port_of(GPIO_PB09)->PUPDR, 9));
  TEST_ASSERT_BITS_HIGH((1u << 9), port_of(GPIO_PB09)->OTYPER);
  TEST_ASSERT_EQUAL_UINT32(2u, field2(port_of(GPIO_PB09)->OSPEEDR, 9));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_GPIO_AF7,
                           (port_of(GPIO_PB09)->AFRH >> (4 * 1)) & 0xFu);
}

void test_host_gpio_init_rejects_null(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_gpio_init(GPIO_PA00, NULL));
}

NAVTEST_CASE_DECL(test_host_gpio_port_indexing);
NAVTEST_CASE_DECL(test_host_gpio_enable_clock_all_ports);
NAVTEST_CASE_DECL(test_host_gpio_set_mode_all_pins_all_modes);
NAVTEST_CASE_DECL(test_host_gpio_set_mode_writes_pupdr);
NAVTEST_CASE_DECL(test_host_gpio_get_mode_round_trip);
NAVTEST_CASE_DECL(test_host_gpio_alternate_function_all_pins);
NAVTEST_CASE_DECL(test_host_gpio_output_type);
NAVTEST_CASE_DECL(test_host_gpio_output_speed);
NAVTEST_CASE_DECL(test_host_gpio_write_uses_bsrr);
NAVTEST_CASE_DECL(test_host_gpio_read_uses_idr);
NAVTEST_CASE_DECL(test_host_gpio_toggle_flips_odr);
NAVTEST_CASE_DECL(test_host_gpio_init_applies_config);
NAVTEST_CASE_DECL(test_host_gpio_init_rejects_null);

static const navtest_case_t gpio_driver_cases[] = {
    NAVTEST_CASE(test_host_gpio_port_indexing),
    NAVTEST_CASE(test_host_gpio_enable_clock_all_ports),
    NAVTEST_CASE(test_host_gpio_set_mode_all_pins_all_modes),
    NAVTEST_CASE(test_host_gpio_set_mode_writes_pupdr),
    NAVTEST_CASE(test_host_gpio_get_mode_round_trip),
    NAVTEST_CASE(test_host_gpio_alternate_function_all_pins),
    NAVTEST_CASE(test_host_gpio_output_type),
    NAVTEST_CASE(test_host_gpio_output_speed),
    NAVTEST_CASE(test_host_gpio_write_uses_bsrr),
    NAVTEST_CASE(test_host_gpio_read_uses_idr),
    NAVTEST_CASE(test_host_gpio_toggle_flips_odr),
    NAVTEST_CASE(test_host_gpio_init_applies_config),
    NAVTEST_CASE(test_host_gpio_init_rejects_null),
};

const navtest_suite_t test_gpio_driver_suite = {
    .name = "GPIO DRIVER (host)",
    .cases = gpio_driver_cases,
    .count = sizeof(gpio_driver_cases) / sizeof(gpio_driver_cases[0]),
    .between = NULL,
};
