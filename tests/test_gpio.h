/**
 * @file tests/test_gpio.h
 * @brief Standardized hal_gpio_* API tests.
 */

#ifndef TEST_GPIO_H
#define TEST_GPIO_H

#include "navtest/navtest.h"
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
/* All tests target this pin (PC09 — unused on Nucleo-F401RE board header). */
#define TEST_GPIO_PIN GPIO_PC09
#define TEST_GPIO_AF  HAL_GPIO_AF7

void test_hal_gpio_init_applies_config(void);
void test_hal_gpio_init_rejects_null_config(void);
void test_hal_gpio_set_mode_writes_moder(void);
void test_hal_gpio_get_mode_round_trip(void);
void test_hal_gpio_enable_clock_sets_ahb1_bit(void);
void test_hal_gpio_set_alternate_function_writes_af(void);
void test_hal_gpio_set_alternate_function_switches_mode_to_af(void);
void test_hal_gpio_set_output_type_writes_otyper(void);
void test_hal_gpio_set_output_speed_writes_ospeedr(void);
void test_hal_gpio_write_high_then_low(void);
void test_hal_gpio_toggle_flips_state(void);

extern const navtest_suite_t test_gpio_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_GPIO_H
