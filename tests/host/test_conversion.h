#ifndef TEST_HOST_CONVERSION_H
#define TEST_HOST_CONVERSION_H

#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
void test_str_to_int_basic(void);
void test_str_to_int_negative(void);
void test_str_to_int_plus_sign(void);
void test_str_to_int_leading_whitespace(void);
void test_str_to_int_stops_at_non_digit(void);
void test_str_to_int_empty_string(void);

void test_str_to_float_basic(void);
void test_str_to_float_with_decimal(void);
void test_str_to_float_negative(void);

extern const navtest_suite_t test_conversion_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
