#ifndef TEST_HOST_GPIO_ENCODING_H
#define TEST_HOST_GPIO_ENCODING_H

#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
void test_gpio_pin_encoding_layout(void);
void test_gpio_get_pin_returns_low_nibble(void);
void test_gpio_get_port_number_for_a_through_e(void);
void test_gpio_get_port_number_skips_to_h(void);

extern const navtest_suite_t test_gpio_encoding_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
