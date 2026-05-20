#ifndef TEST_GPIO_H
#define TEST_GPIO_H
#include "navtest/navtest.h"
#include <stdint.h>

// Test pin and AF values
#define TEST_PIN GPIO_PC09
#define TEST_AF HAL_GPIO_AF7

// Test function declarations
void test_hal_gpio_setmode(void);
void test_hal_gpio_getmode(void);
void test_hal_gpio_digitalwrite_sets_pin_high(void);
void test_hal_gpio_digitalwrite_sets_pin_low(void);
void test_gpio_set_alternate_function(void);

extern const navtest_suite_t test_gpio_suite;

#endif // TEST_GPIO_H
