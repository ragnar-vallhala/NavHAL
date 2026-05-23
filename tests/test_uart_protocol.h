/**
 * @file tests/test_uart_protocol.h
 * @brief Standardized hal_uart_* API tests.
 *
 * Tests target UART1 / UART6 — UART2 is the test-console UART and must
 * not be reconfigured by the suite.
 */

#ifndef TEST_UART_PROTOCOL_H
#define TEST_UART_PROTOCOL_H

#include "navtest/navtest.h"


#ifdef __cplusplus
extern "C" {
#endif
/* baud-rate / init tests */
void test_uart_baudrate_9600(void);
void test_uart_baudrate_115200(void);
void test_hal_uart_init_returns_ok(void);
void test_hal_uart_init_rejects_null_config(void);

/* write surface */
void test_hal_uart_write_returns_ok(void);
void test_hal_uart_write_char_returns_ok(void);
void test_hal_uart_write_int_returns_ok(void);
void test_hal_uart_write_uint_returns_ok(void);
void test_hal_uart_write_float_returns_ok(void);
void test_hal_uart_write_string_returns_ok(void);
void test_hal_uart_print_generic_dispatch(void);

/* read surface */
void test_hal_uart_available_after_init_is_false(void);

extern const navtest_suite_t test_uart_protocol_suite;


#ifdef __cplusplus
} /* extern "C" */
#endif
#endif // TEST_UART_PROTOCOL_H
