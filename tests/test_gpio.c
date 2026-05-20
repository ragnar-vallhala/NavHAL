#define CORTEX_M4
#include "core/cortex-m4/gpio.h"
#include "core/cortex-m4/gpio_reg.h"
#include "navtest/navtest.h"
#include <stdint.h>

#define TEST_PIN GPIO_PC09
#define TEST_AF HAL_GPIO_AF7

void test_hal_gpio_setmode(void) {
  hal_gpio_set_mode(TEST_PIN, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
  TEST_ASSERT_EQUAL_UINT32(
      HAL_GPIO_MODE_OUTPUT,
      (GPIO_GET_PORT(TEST_PIN)->MODER >> (GPIO_GET_PIN(TEST_PIN) * 2)) & 0x3);
}

void test_hal_gpio_getmode(void) {
  hal_gpio_set_mode(TEST_PIN, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
  TEST_ASSERT_EQUAL_UINT32(
      hal_gpio_get_mode(TEST_PIN),
      (GPIO_GET_PORT(TEST_PIN)->MODER >> (GPIO_GET_PIN(TEST_PIN) * 2)) & 0x3);
}

void test_hal_gpio_digitalwrite_sets_pin_high(void) {
  hal_gpio_set_mode(TEST_PIN, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
  hal_gpio_write(TEST_PIN, HAL_GPIO_HIGH);
  TEST_ASSERT_EQUAL_UINT32(
      1, (GPIO_GET_PORT(TEST_PIN)->ODR >> GPIO_GET_PIN(TEST_PIN)) & 0x1);
}

void test_hal_gpio_digitalwrite_sets_pin_low(void) {
  hal_gpio_set_mode(TEST_PIN, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
  hal_gpio_write(TEST_PIN, HAL_GPIO_LOW);
  TEST_ASSERT_EQUAL_UINT32(
      0, (GPIO_GET_PORT(TEST_PIN)->ODR >> GPIO_GET_PIN(TEST_PIN)) & 0x1);
}

void test_gpio_set_alternate_function(void) {
  hal_gpio_set_alternate_function(TEST_PIN, TEST_AF);
  if (GPIO_GET_PIN(TEST_PIN) < 8)
    TEST_ASSERT_EQUAL_UINT32(TEST_AF, (GPIO_GET_PORT(TEST_PIN)->AFRL >>
                                       ((4 * (GPIO_GET_PIN(TEST_PIN) % 8)))) &
                                          0xf);
  else
    TEST_ASSERT_EQUAL_UINT32(TEST_AF, (GPIO_GET_PORT(TEST_PIN)->AFRH >>
                                       ((4 * (GPIO_GET_PIN(TEST_PIN) % 8)))) &
                                          0xf);
}

static const navtest_case_t gpio_cases[] = {
    NAVTEST_CASE(test_hal_gpio_setmode),
    NAVTEST_CASE(test_hal_gpio_getmode),
    NAVTEST_CASE(test_hal_gpio_digitalwrite_sets_pin_high),
    NAVTEST_CASE(test_hal_gpio_digitalwrite_sets_pin_low),
    NAVTEST_CASE(test_gpio_set_alternate_function),
};

const navtest_suite_t test_gpio_suite = {
    .name = "GPIO",
    .cases = gpio_cases,
    .count = sizeof(gpio_cases) / sizeof(gpio_cases[0]),
    .between = NULL,
};
