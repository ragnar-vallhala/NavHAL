#include "core/cortex-m4/gpio_reg.h"
#include "core/cortex-m4/uart.h"
#include "utils/gpio_types.h"
#define CORTEX_M4
#include "navhal.h"
#include "unity.h"
#include <stdint.h>

int putchar(int ch) {
  uart2_write_char(ch);
  return ch;
}
void abort(void) {
  while (1) {
  }
}

void *memcpy(void *dest, const void *src, unsigned int n) {
  char *d = dest;
  const char *s = src;
  while (n--) {
    *d++ = *s++;
  }
  return dest;
}

void setUp(void) { uart2_init(9600); }
void tearDown(void) {}
// Mock hal_gpio_digitalwrite to manipulate GPIO_PORT for testing

#include "unity.h"

void test_hal_gpio_digitalwrite_sets_pin_high(void) {
  GPIO_GET_PORT(GPIO_PA05)->BSRR = 0;
  hal_gpio_digitalwrite(GPIO_PA05, GPIO_HIGH);
  TEST_ASSERT_EQUAL_UINT32(1 << 5, GPIO_GET_PORT(GPIO_PA05)->BSRR);
}

void test_hal_gpio_digitalwrite_sets_pin_low(void) {
  GPIO_GET_PORT(GPIO_PA05)->BSRR = 0;
  hal_gpio_digitalwrite(GPIO_PA05, GPIO_LOW);
  TEST_ASSERT_EQUAL_UINT32(1 << (5 + 16), GPIO_GET_PORT(GPIO_PA05)->BSRR);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_hal_gpio_digitalwrite_sets_pin_high);
  RUN_TEST(test_hal_gpio_digitalwrite_sets_pin_low);
  return UNITY_END();
}
