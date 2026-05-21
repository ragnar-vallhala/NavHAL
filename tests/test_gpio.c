/**
 * @file tests/test_gpio.c
 * @brief Standardized hal_gpio_* API tests (success + error paths).
 */

#define CORTEX_M4
#include "navhal_port_gpio.h"
#include "family/gpio_reg.h"
#include "family/rcc_reg.h"
#include "navtest/navtest.h"
#include "navtest/navtest_pil.h"
#include "test_gpio.h"

#include <stdint.h>

/* MODER / OTYPER / OSPEEDR / AFR slice helpers — read back the bits we
 * just wrote so each test asserts on the actual register state. */
static uint32_t moder_bits(hal_gpio_pin_t pin) {
  return (GPIO_GET_PORT(pin)->MODER >> (GPIO_GET_PIN(pin) * 2)) & 0x3u;
}

static uint32_t otyper_bit(hal_gpio_pin_t pin) {
  return (GPIO_GET_PORT(pin)->OTYPER >> GPIO_GET_PIN(pin)) & 0x1u;
}

static uint32_t ospeedr_bits(hal_gpio_pin_t pin) {
  return (GPIO_GET_PORT(pin)->OSPEEDR >> (GPIO_GET_PIN(pin) * 2)) & 0x3u;
}

static uint32_t af_nibble(hal_gpio_pin_t pin) {
  uint8_t p = GPIO_GET_PIN(pin);
  if (p < 8)
    return (GPIO_GET_PORT(pin)->AFRL >> (4 * (p % 8))) & 0xFu;
  return (GPIO_GET_PORT(pin)->AFRH >> (4 * (p % 8))) & 0xFu;
}

static uint32_t odr_bit(hal_gpio_pin_t pin) {
  return (GPIO_GET_PORT(pin)->ODR >> GPIO_GET_PIN(pin)) & 0x1u;
}

/* GPIOC sits at AHB1ENR bit 2 (port-number 0..7 = bit 0..7). */
#define GPIOC_AHB1ENR_BIT (1U << 2)

/* -------------------- Success-path tests -------------------- */

void test_hal_gpio_init_applies_config(void) {
  hal_gpio_config_t cfg = {
      .mode = HAL_GPIO_MODE_OUTPUT,
      .pull = HAL_GPIO_PULL_NONE,
      .output_type = HAL_GPIO_OTYPE_PUSH_PULL,
      .output_speed = HAL_GPIO_SPEED_HIGH,
      .alternate = HAL_GPIO_AF0,
  };
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_gpio_init(TEST_GPIO_PIN, &cfg));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_GPIO_MODE_OUTPUT,
                           moder_bits(TEST_GPIO_PIN));
  TEST_ASSERT_EQUAL_UINT32(0u, otyper_bit(TEST_GPIO_PIN));
  TEST_ASSERT_EQUAL_UINT32(2u, ospeedr_bits(TEST_GPIO_PIN)); /* HIGH = 2 */
}

void test_hal_gpio_set_mode_writes_moder(void) {
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_OK,
      (uint32_t)hal_gpio_set_mode(TEST_GPIO_PIN, HAL_GPIO_MODE_OUTPUT,
                                  HAL_GPIO_PULL_NONE));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_GPIO_MODE_OUTPUT,
                           moder_bits(TEST_GPIO_PIN));

  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_OK,
      (uint32_t)hal_gpio_set_mode(TEST_GPIO_PIN, HAL_GPIO_MODE_INPUT,
                                  HAL_GPIO_PULL_UP));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_GPIO_MODE_INPUT,
                           moder_bits(TEST_GPIO_PIN));
}

void test_hal_gpio_get_mode_round_trip(void) {
  hal_gpio_set_mode(TEST_GPIO_PIN, HAL_GPIO_MODE_ANALOG, HAL_GPIO_PULL_NONE);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_GPIO_MODE_ANALOG,
                           (uint32_t)hal_gpio_get_mode(TEST_GPIO_PIN));
  hal_gpio_set_mode(TEST_GPIO_PIN, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_GPIO_MODE_OUTPUT,
                           (uint32_t)hal_gpio_get_mode(TEST_GPIO_PIN));
}

void test_hal_gpio_enable_clock_sets_ahb1_bit(void) {
  /* Clear, then enable, then check the AHB1ENR bit for port C. */
  RCC->AHB1ENR &= ~GPIOC_AHB1ENR_BIT;
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_gpio_enable_clock(TEST_GPIO_PIN));
  TEST_ASSERT_BITS_HIGH(GPIOC_AHB1ENR_BIT, RCC->AHB1ENR);
}

void test_hal_gpio_set_alternate_function_writes_af(void) {
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_OK,
      (uint32_t)hal_gpio_set_alternate_function(TEST_GPIO_PIN, TEST_GPIO_AF));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)TEST_GPIO_AF, af_nibble(TEST_GPIO_PIN));
}

void test_hal_gpio_set_alternate_function_switches_mode_to_af(void) {
  hal_gpio_set_mode(TEST_GPIO_PIN, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
  hal_gpio_set_alternate_function(TEST_GPIO_PIN, TEST_GPIO_AF);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_GPIO_MODE_AF,
                           moder_bits(TEST_GPIO_PIN));
}

void test_hal_gpio_set_output_type_writes_otyper(void) {
  NAVTEST_SKIP_ON_PIL();
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_gpio_set_output_type(
                               TEST_GPIO_PIN, HAL_GPIO_OTYPE_OPEN_DRAIN));
  TEST_ASSERT_EQUAL_UINT32(1u, otyper_bit(TEST_GPIO_PIN));

  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_gpio_set_output_type(
                               TEST_GPIO_PIN, HAL_GPIO_OTYPE_PUSH_PULL));
  TEST_ASSERT_EQUAL_UINT32(0u, otyper_bit(TEST_GPIO_PIN));
}

void test_hal_gpio_set_output_speed_writes_ospeedr(void) {
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_OK,
      (uint32_t)hal_gpio_set_output_speed(TEST_GPIO_PIN,
                                          HAL_GPIO_SPEED_VERY_HIGH));
  TEST_ASSERT_EQUAL_UINT32(3u, ospeedr_bits(TEST_GPIO_PIN));
}

void test_hal_gpio_write_high_then_low(void) {
  hal_gpio_set_mode(TEST_GPIO_PIN, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
  hal_gpio_write(TEST_GPIO_PIN, HAL_GPIO_HIGH);
  TEST_ASSERT_EQUAL_UINT32(1u, odr_bit(TEST_GPIO_PIN));
  hal_gpio_write(TEST_GPIO_PIN, HAL_GPIO_LOW);
  TEST_ASSERT_EQUAL_UINT32(0u, odr_bit(TEST_GPIO_PIN));
}

void test_hal_gpio_toggle_flips_state(void) {
  hal_gpio_set_mode(TEST_GPIO_PIN, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
  hal_gpio_write(TEST_GPIO_PIN, HAL_GPIO_LOW);
  uint32_t before = odr_bit(TEST_GPIO_PIN);
  hal_gpio_toggle(TEST_GPIO_PIN);
  TEST_ASSERT_EQUAL_UINT32(before ^ 1u, odr_bit(TEST_GPIO_PIN));
  hal_gpio_toggle(TEST_GPIO_PIN);
  TEST_ASSERT_EQUAL_UINT32(before, odr_bit(TEST_GPIO_PIN));
}

/* -------------------- Error-path tests -------------------- */

void test_hal_gpio_init_rejects_null_config(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_gpio_init(TEST_GPIO_PIN, NULL));
}

/* -------------------- Suite -------------------- */

static const navtest_case_t gpio_cases[] = {
    NAVTEST_CASE(test_hal_gpio_init_applies_config),
    NAVTEST_CASE(test_hal_gpio_set_mode_writes_moder),
    NAVTEST_CASE(test_hal_gpio_get_mode_round_trip),
    NAVTEST_CASE(test_hal_gpio_enable_clock_sets_ahb1_bit),
    NAVTEST_CASE(test_hal_gpio_set_alternate_function_writes_af),
    NAVTEST_CASE(test_hal_gpio_set_alternate_function_switches_mode_to_af),
    NAVTEST_CASE(test_hal_gpio_set_output_type_writes_otyper),
    NAVTEST_CASE(test_hal_gpio_set_output_speed_writes_ospeedr),
    NAVTEST_CASE(test_hal_gpio_write_high_then_low),
    NAVTEST_CASE(test_hal_gpio_toggle_flips_state),
    /* error paths */
    NAVTEST_CASE(test_hal_gpio_init_rejects_null_config),
};

const navtest_suite_t test_gpio_suite = {
    .name = "GPIO",
    .cases = gpio_cases,
    .count = sizeof(gpio_cases) / sizeof(gpio_cases[0]),
    .between = NULL,
};
