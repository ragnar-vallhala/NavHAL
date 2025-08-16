#ifndef TEST_GPIO_H
#define TEST_GPIO_H
#include "unity.h"
#include <stdint.h>

// Test pin and AF values
#define TEST_PIN GPIO_PC09
#define TEST_AF GPIO_AF07

// Test function declarations
void test_hal_gpio_setmode(void);
void test_hal_gpio_getmode(void);
void test_hal_gpio_digitalwrite_sets_pin_high(void);
void test_hal_gpio_digitalwrite_sets_pin_low(void);
void test_gpio_set_alternate_function(void);

#endif // TEST_GPIO_H
