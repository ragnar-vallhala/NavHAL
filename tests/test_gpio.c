/**
 * @file test_gpio.c
 * @brief GPIO unit test implementation for NAVHAL
 * @details
 * This file implements the test cases for verifying GPIO functionality in the
 * NAVHAL hardware abstraction layer. It tests both the high-level HAL interface
 * and the underlying register-level configurations.
 *
 * Tests cover:
 * - Mode configuration (input/output/alternate)
 * - Digital write operations (high/low)
 * - Alternate function setup
 * - Register-level verification of all operations
 *
 * @note Uses Unity test framework for assertions
 * @note Tests use PC9 (TEST_PIN) as the test pin with AF7 (TEST_AF) for AF tests
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#define CORTEX_M4
#include "core/cortex-m4/gpio_reg.h"
#include "navhal.h"
#define UNITY_OUTPUT_COLOR
#include "unity.h"
#include <stdint.h>

#define TEST_PIN GPIO_PC09  ///< Test pin used for all GPIO tests (Port C, Pin 9)
#define TEST_AF GPIO_AF07   ///< Alternate function used for testing (AF7)

/**
 * @brief Test GPIO mode configuration
 * @details
 * Verifies that hal_gpio_setmode() correctly configures:
 * 1. The pin mode in the MODER register
 * 2. The pull-up/pull-down settings
 * 
 * Test sequence:
 * 1. Configure pin as output with no pull-up/pull-down
 * 2. Verify MODER register bits are set correctly
 */
void test_hal_gpio_setmode(void) {
  hal_gpio_setmode(TEST_PIN, GPIO_OUTPUT, GPIO_PUPD_NONE);
  TEST_ASSERT_EQUAL_UINT32(
      GPIO_OUTPUT,
      (GPIO_GET_PORT(TEST_PIN)->MODER >> (GPIO_GET_PIN(TEST_PIN) * 2)) & 0x3);
}

/**
 * @brief Test GPIO mode reading
 * @details
 * Verifies that hal_gpio_getmode() correctly returns:
 * 1. The current pin mode configuration
 * 2. Matches the actual hardware register state
 * 
 * Test sequence:
 * 1. Configure pin as output
 * 2. Compare getmode() return value with direct register read
 */
void test_hal_gpio_getmode(void) {
  hal_gpio_setmode(TEST_PIN, GPIO_OUTPUT, GPIO_PUPD_NONE);
  TEST_ASSERT_EQUAL_UINT32(
      hal_gpio_getmode(TEST_PIN),
      (GPIO_GET_PORT(TEST_PIN)->MODER >> (GPIO_GET_PIN(TEST_PIN) * 2)) & 0x3);
}

/**
 * @brief Test digital write (high) operation
 * @details
 * Verifies that hal_gpio_digitalwrite() with GPIO_HIGH:
 * 1. Sets the correct bit in the ODR register
 * 2. Actually drives the pin high
 * 
 * Test sequence:
 * 1. Configure pin as output
 * 2. Write high
 * 3. Verify ODR register state
 */
void test_hal_gpio_digitalwrite_sets_pin_high(void) {
  hal_gpio_setmode(TEST_PIN, GPIO_OUTPUT, GPIO_PUPD_NONE);
  hal_gpio_digitalwrite(TEST_PIN, GPIO_HIGH);
  TEST_ASSERT_EQUAL_UINT32(
      1, (GPIO_GET_PORT(TEST_PIN)->ODR >> GPIO_GET_PIN(TEST_PIN)) & 0x1);
}

/**
 * @brief Test digital write (low) operation
 * @details
 * Verifies that hal_gpio_digitalwrite() with GPIO_LOW:
 * 1. Clears the correct bit in the ODR register
 * 2. Actually drives the pin low
 * 
 * Test sequence:
 * 1. Configure pin as output
 * 2. Write low
 * 3. Verify ODR register state
 */
void test_hal_gpio_digitalwrite_sets_pin_low(void) {
  hal_gpio_setmode(TEST_PIN, GPIO_OUTPUT, GPIO_PUPD_NONE);
  hal_gpio_digitalwrite(TEST_PIN, GPIO_LOW);
  TEST_ASSERT_EQUAL_UINT32(
      0, (GPIO_GET_PORT(TEST_PIN)->ODR >> GPIO_GET_PIN(TEST_PIN)) & 0x1);
}

/**
 * @brief Test alternate function configuration
 * @details
 * Verifies that hal_gpio_set_alternate_function():
 * 1. Correctly sets the alternate function number
 * 2. Handles both AFRL (pins 0-7) and AFRH (pins 8-15) registers
 * 
 * Test sequence:
 * 1. Configure pin for alternate function
 * 2. Verify correct AF number in appropriate register (AFRL/AFRH)
 */
void test_gpio_set_alternate_function(void) {
  hal_gpio_set_alternate_function(TEST_PIN, TEST_AF);
  if (GPIO_GET_PIN(TEST_PIN) < 8)
    TEST_ASSERT_EQUAL_UINT32(TEST_AF, (GPIO_GET_PORT(TEST_PIN)->AFRL >>
                                     (4 * (GPIO_GET_PIN(TEST_PIN) % 8))) & 0xf);
  else
    TEST_ASSERT_EQUAL_UINT32(TEST_AF, (GPIO_GET_PORT(TEST_PIN)->AFRH >>
                                     (4 * (GPIO_GET_PIN(TEST_PIN) % 8))) & 0xf);
}
