/**
 * @file tests/host/test_gpio_encoding.c
 * @brief Host-runnable tests for the GPIO pin encoding contract.
 *
 * The `GPIO_Pxnn` enum is dense (A=0..15, B=16..31, ...), and the
 * macros `GPIO_GET_PIN` / `GPIO_GET_PORT_NUMBER` derive the port and
 * pin from that encoding. Locking this contract on the host is M6
 * (AVR) insurance: the AVR port redefines the enum for ports B/C/D
 * only, and any drift in the encoder math would invalidate every
 * driver that touches a pin.
 */

#include "family/gpio_reg.h"
#include "test_gpio_encoding.h"
#include "utils/gpio_types.h"

void test_gpio_pin_encoding_layout(void) {
  /* Dense, port-major layout — verify a couple of anchor pins. */
  TEST_ASSERT_EQUAL_UINT32(0, (uint32_t)GPIO_PA00);
  TEST_ASSERT_EQUAL_UINT32(5, (uint32_t)GPIO_PA05);
  TEST_ASSERT_EQUAL_UINT32(15, (uint32_t)GPIO_PA15);
  TEST_ASSERT_EQUAL_UINT32(16, (uint32_t)GPIO_PB00);
  TEST_ASSERT_EQUAL_UINT32(20, (uint32_t)GPIO_PB04);
  TEST_ASSERT_EQUAL_UINT32(32, (uint32_t)GPIO_PC00);
  TEST_ASSERT_EQUAL_UINT32(48, (uint32_t)GPIO_PD00);
  TEST_ASSERT_EQUAL_UINT32(64, (uint32_t)GPIO_PE00);
  TEST_ASSERT_EQUAL_UINT32(80, (uint32_t)GPIO_PH00);
}

void test_gpio_get_pin_returns_low_nibble(void) {
  TEST_ASSERT_EQUAL_UINT32(0, GPIO_GET_PIN(GPIO_PA00));
  TEST_ASSERT_EQUAL_UINT32(5, GPIO_GET_PIN(GPIO_PA05));
  TEST_ASSERT_EQUAL_UINT32(15, GPIO_GET_PIN(GPIO_PA15));
  TEST_ASSERT_EQUAL_UINT32(4, GPIO_GET_PIN(GPIO_PB04));
  TEST_ASSERT_EQUAL_UINT32(9, GPIO_GET_PIN(GPIO_PC09));
  TEST_ASSERT_EQUAL_UINT32(9, GPIO_GET_PIN(GPIO_PH09));
}

void test_gpio_get_port_number_for_a_through_e(void) {
  TEST_ASSERT_EQUAL_UINT32(0, GPIO_GET_PORT_NUMBER(GPIO_PA00));
  TEST_ASSERT_EQUAL_UINT32(0, GPIO_GET_PORT_NUMBER(GPIO_PA15));
  TEST_ASSERT_EQUAL_UINT32(1, GPIO_GET_PORT_NUMBER(GPIO_PB00));
  TEST_ASSERT_EQUAL_UINT32(2, GPIO_GET_PORT_NUMBER(GPIO_PC09));
  TEST_ASSERT_EQUAL_UINT32(3, GPIO_GET_PORT_NUMBER(GPIO_PD00));
  TEST_ASSERT_EQUAL_UINT32(4, GPIO_GET_PORT_NUMBER(GPIO_PE15));
}

void test_gpio_get_port_number_skips_to_h(void) {
  /* Port H lands at port-number 7 because the base-address layout reserves
   * slots for the missing F and G ports on the F401RE. The encoder special-
   * cases the H-port jump — verify it. */
  TEST_ASSERT_EQUAL_UINT32(7, GPIO_GET_PORT_NUMBER(GPIO_PH00));
  TEST_ASSERT_EQUAL_UINT32(7, GPIO_GET_PORT_NUMBER(GPIO_PH09));
}

static const navtest_case_t gpio_encoding_cases[] = {
    NAVTEST_CASE(test_gpio_pin_encoding_layout),
    NAVTEST_CASE(test_gpio_get_pin_returns_low_nibble),
    NAVTEST_CASE(test_gpio_get_port_number_for_a_through_e),
    NAVTEST_CASE(test_gpio_get_port_number_skips_to_h),
};

const navtest_suite_t test_gpio_encoding_suite = {
    .name = "GPIO ENCODING (host)",
    .cases = gpio_encoding_cases,
    .count = sizeof(gpio_encoding_cases) / sizeof(gpio_encoding_cases[0]),
    .between = NULL,
};
