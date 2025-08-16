/**
 * @file test_gpio.h
 * @brief GPIO unit test declarations for NAVHAL
 * @details
 * This header defines the test cases for verifying GPIO functionality in the
 * NAVHAL hardware abstraction layer. It includes tests for:
 * - Mode configuration (input/output/alternate)
 * - Digital write operations
 * - Alternate function setup
 *
 * @note Uses Unity test framework for assertions
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef TEST_GPIO_H
#define TEST_GPIO_H

#include "unity.h"
#include <stdint.h>

/**
 * @def TEST_PIN
 * @brief GPIO pin used for all tests (PC9)
 */
#define TEST_PIN GPIO_PC09

/**
 * @def TEST_AF
 * @brief Alternate function number used for testing (AF7)
 */
#define TEST_AF GPIO_AF07

/**
 * @brief Test GPIO mode configuration
 * @details
 * Verifies hal_gpio_setmode() correctly configures:
 * - Input/output modes
 * - Pull-up/pull-down settings
 */
void test_hal_gpio_setmode(void);

/**
 * @brief Test GPIO mode reading
 * @details
 * Verifies hal_gpio_getmode() correctly returns:
 * - Currently configured mode
 * - Matches what was set with hal_gpio_setmode()
 */
void test_hal_gpio_getmode(void);

/**
 * @brief Test digital write high
 * @details
 * Verifies hal_gpio_digitalwrite() correctly:
 * - Sets pin to logic high
 * - Maintains state until changed
 */
void test_hal_gpio_digitalwrite_sets_pin_high(void);

/**
 * @brief Test digital write low
 * @details
 * Verifies hal_gpio_digitalwrite() correctly:
 * - Sets pin to logic low
 * - Maintains state until changed
 */
void test_hal_gpio_digitalwrite_sets_pin_low(void);

/**
 * @brief Test alternate function configuration
 * @details
 * Verifies hal_gpio_set_alternate_function() correctly:
 * - Configures pin for alternate function
 * - Sets the specified function number
 * - Maintains other pin settings
 */
void test_gpio_set_alternate_function(void);

#endif // TEST_GPIO_H