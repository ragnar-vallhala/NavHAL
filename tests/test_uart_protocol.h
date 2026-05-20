#ifndef TEST_UART_PROTOCOL_H
#define TEST_UART_PROTOCOL_H

#include "navtest/navtest.h"

void test_uart_baudrate_9600(void);
void test_uart_baudrate_115200(void);

extern const navtest_suite_t test_uart_protocol_suite;

#endif // TEST_UART_PROTOCOL_H
