/**
 * @file tests/host/test_conversion.c
 * @brief Host-runnable tests for the string-to-number utilities.
 */

#include "test_conversion.h"
#include "utils/conversion.h"

void test_str_to_int_basic(void) {
  TEST_ASSERT_EQUAL_UINT32(1234, (uint32_t)str_to_int("1234"));
  TEST_ASSERT_EQUAL_UINT32(0, (uint32_t)str_to_int("0"));
}

void test_str_to_int_negative(void) {
  /* compare signed via cast through int32_t */
  int32_t v = str_to_int("-42");
  TEST_ASSERT_EQUAL_UINT32((uint32_t)(int32_t)-42, (uint32_t)v);
}

void test_str_to_int_plus_sign(void) {
  TEST_ASSERT_EQUAL_UINT32(7, (uint32_t)str_to_int("+7"));
}

void test_str_to_int_leading_whitespace(void) {
  TEST_ASSERT_EQUAL_UINT32(42, (uint32_t)str_to_int("   42"));
  TEST_ASSERT_EQUAL_UINT32(42, (uint32_t)str_to_int("\t42"));
}

void test_str_to_int_stops_at_non_digit(void) {
  TEST_ASSERT_EQUAL_UINT32(1234, (uint32_t)str_to_int("1234abc"));
  /* documented partial-parse behavior */
}

void test_str_to_int_empty_string(void) {
  TEST_ASSERT_EQUAL_UINT32(0, (uint32_t)str_to_int(""));
}

/* float comparisons use a small epsilon to absorb the implementation's
 * limited fractional precision (documented in conversion.h). */
static int float_near(float a, float b) {
  float d = a - b;
  if (d < 0) d = -d;
  return d < 0.001f;
}

void test_str_to_float_basic(void) {
  TEST_ASSERT_TRUE(float_near(42.0f, str_to_float("42")));
}

void test_str_to_float_with_decimal(void) {
  TEST_ASSERT_TRUE(float_near(3.14f, str_to_float("3.14")));
}

void test_str_to_float_negative(void) {
  TEST_ASSERT_TRUE(float_near(-2.5f, str_to_float("-2.5")));
}

static const navtest_case_t conversion_cases[] = {
    NAVTEST_CASE(test_str_to_int_basic),
    NAVTEST_CASE(test_str_to_int_negative),
    NAVTEST_CASE(test_str_to_int_plus_sign),
    NAVTEST_CASE(test_str_to_int_leading_whitespace),
    NAVTEST_CASE(test_str_to_int_stops_at_non_digit),
    NAVTEST_CASE(test_str_to_int_empty_string),
    NAVTEST_CASE(test_str_to_float_basic),
    NAVTEST_CASE(test_str_to_float_with_decimal),
    NAVTEST_CASE(test_str_to_float_negative),
};

const navtest_suite_t test_conversion_suite = {
    .name = "CONVERSION (host)",
    .cases = conversion_cases,
    .count = sizeof(conversion_cases) / sizeof(conversion_cases[0]),
    .between = NULL,
};
