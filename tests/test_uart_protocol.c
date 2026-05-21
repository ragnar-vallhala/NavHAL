/**
 * @file tests/test_uart_protocol.c
 * @brief Standardized hal_uart_* API tests.
 *
 * Tests target UART1 / UART6 — UART2 is the test-console UART and must
 * not be reconfigured by the suite. Single-byte writes go to the wire
 * but have no listener on these boards, so they exit immediately when
 * the TX register frees.
 */

#define CORTEX_M4
#include "test_uart_protocol.h"
#include "navhal_port_clock.h"
#include "navhal_port_uart.h"
#include "family/uart_reg.h"
#include "navtest/navtest.h"
#include "navtest/navtest_pil.h"

#include <stddef.h>
#include <stdint.h>

#define TEST_UART HAL_UART_1

static void init_test_uart(uint32_t baud) {
  hal_uart_config_t cfg = {.baudrate = baud};
  hal_uart_init(TEST_UART, &cfg);
}

/* -------------------- baud-rate / BRR tests -------------------- */

void test_uart_baudrate_9600(void) {
  hal_uart_init(HAL_UART_1, &(hal_uart_config_t){.baudrate = 9600});
  volatile UARTx_Reg_Typedef *u1 = GET_USARTx_BASE(1);
  uint32_t pclk = hal_clock_get_apb2clk();
  uint32_t expected_brr = (pclk + 4800) / 9600;
  TEST_ASSERT_EQUAL_UINT32(expected_brr, u1->BRR);
}

void test_uart_baudrate_115200(void) {
  /* Renode's UART6 model doesn't reflect BRR writes back to the register;
   * the same test passes for UART1 above and on HIL for UART6. */
  NAVTEST_SKIP_ON_PIL();
  hal_uart_init(HAL_UART_6, &(hal_uart_config_t){.baudrate = 115200});
  volatile UARTx_Reg_Typedef *u6 = GET_USARTx_BASE(6);
  uint32_t pclk = hal_clock_get_apb2clk();
  uint32_t expected_brr = (pclk + 57600) / 115200;
  TEST_ASSERT_EQUAL_UINT32(expected_brr, u6->BRR);
}

/* -------------------- init contract -------------------- */

void test_hal_uart_init_returns_ok(void) {
  hal_uart_config_t cfg = {.baudrate = 9600};
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_uart_init(TEST_UART, &cfg));
}

void test_hal_uart_init_rejects_null_config(void) {
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_ERR_INVALID_ARG,
                           (uint32_t)hal_uart_init(TEST_UART, NULL));
}

/* -------------------- write surface -------------------- */

void test_hal_uart_write_returns_ok(void) {
  init_test_uart(115200);
  const uint8_t buf[] = {'A', 'B', 'C'};
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_OK,
      (uint32_t)hal_uart_write(TEST_UART, buf, sizeof(buf)));
}

void test_hal_uart_write_char_returns_ok(void) {
  init_test_uart(115200);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_uart_write_char(TEST_UART, 'X'));
}

void test_hal_uart_write_int_returns_ok(void) {
  init_test_uart(115200);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_uart_write_int(TEST_UART, -42));
}

void test_hal_uart_write_uint_returns_ok(void) {
  init_test_uart(115200);
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_uart_write_uint(TEST_UART, 42u));
}

void test_hal_uart_write_float_returns_ok(void) {
  init_test_uart(115200);
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_OK,
      (uint32_t)hal_uart_write_float(TEST_UART, 3.14f));
}

void test_hal_uart_write_string_returns_ok(void) {
  init_test_uart(115200);
  TEST_ASSERT_EQUAL_UINT32(
      (uint32_t)HAL_OK,
      (uint32_t)hal_uart_write_string(TEST_UART, "navhal"));
}

void test_hal_uart_print_generic_dispatch(void) {
  init_test_uart(115200);
  /* _Generic dispatch — each call must compile to the right type-specific
   * function. A single PASS at runtime is enough; the compile-time check
   * is the actual test. */
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_uart_print(TEST_UART, (int32_t)1));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_uart_print(TEST_UART, (uint32_t)2u));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_uart_print(TEST_UART, 1.5f));
  TEST_ASSERT_EQUAL_UINT32((uint32_t)HAL_OK,
                           (uint32_t)hal_uart_print(TEST_UART, "x"));
}

/* -------------------- read surface -------------------- */

void test_hal_uart_available_after_init_is_false(void) {
  init_test_uart(115200);
  TEST_ASSERT_FALSE(hal_uart_available(TEST_UART));
}

static const navtest_case_t uart_protocol_cases[] = {
    NAVTEST_CASE(test_uart_baudrate_9600),
    NAVTEST_CASE(test_uart_baudrate_115200),
    NAVTEST_CASE(test_hal_uart_init_returns_ok),
    NAVTEST_CASE(test_hal_uart_init_rejects_null_config),
    NAVTEST_CASE(test_hal_uart_write_returns_ok),
    NAVTEST_CASE(test_hal_uart_write_char_returns_ok),
    NAVTEST_CASE(test_hal_uart_write_int_returns_ok),
    NAVTEST_CASE(test_hal_uart_write_uint_returns_ok),
    NAVTEST_CASE(test_hal_uart_write_float_returns_ok),
    NAVTEST_CASE(test_hal_uart_write_string_returns_ok),
    NAVTEST_CASE(test_hal_uart_print_generic_dispatch),
    NAVTEST_CASE(test_hal_uart_available_after_init_is_false),
};

const navtest_suite_t test_uart_protocol_suite = {
    .name = "UART PROTOCOL",
    .cases = uart_protocol_cases,
    .count = sizeof(uart_protocol_cases) / sizeof(uart_protocol_cases[0]),
    .between = NULL,
};
