#define CORTEX_M4
#include "core/cortex-m4/gpio.h"
#include "core/cortex-m4/gpio_reg.h"
#include "unity.h"
#include <stdint.h>

#define TEST_PIN GPIO_PC09
#define TEST_AF GPIO_AF07
void test_hal_gpio_setmode(void) {
  hal_gpio_setmode(TEST_PIN, GPIO_OUTPUT, GPIO_PUPD_NONE);
  TEST_ASSERT_EQUAL_UINT32(
      GPIO_OUTPUT,
      (GPIO_GET_PORT(TEST_PIN)->MODER >> (GPIO_GET_PIN(TEST_PIN) * 2)) & 0x3);
}

void test_hal_gpio_getmode(void) {
  hal_gpio_setmode(TEST_PIN, GPIO_OUTPUT, GPIO_PUPD_NONE);
  TEST_ASSERT_EQUAL_UINT32(
      hal_gpio_getmode(TEST_PIN),
      (GPIO_GET_PORT(TEST_PIN)->MODER >> (GPIO_GET_PIN(TEST_PIN) * 2)) & 0x3);
}

void test_hal_gpio_digitalwrite_sets_pin_high(void) {
  hal_gpio_setmode(TEST_PIN, GPIO_OUTPUT, GPIO_PUPD_NONE);
  hal_gpio_digitalwrite(TEST_PIN, GPIO_HIGH);
  TEST_ASSERT_EQUAL_UINT32(
      1, (GPIO_GET_PORT(TEST_PIN)->ODR >> GPIO_GET_PIN(TEST_PIN)) & 0x1);
}

void test_hal_gpio_digitalwrite_sets_pin_low(void) {
  hal_gpio_setmode(TEST_PIN, GPIO_OUTPUT, GPIO_PUPD_NONE);
  hal_gpio_digitalwrite(TEST_PIN, GPIO_LOW);
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
