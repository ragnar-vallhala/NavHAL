#include "test_uart_protocol.h"
#include "core/cortex-m4/clock.h"
#include "core/cortex-m4/uart.h"
#include "core/cortex-m4/uart_reg.h"
#include "navtest/navtest.h"
#include <stddef.h>
#include <stdint.h>

void test_uart_baudrate_9600(void) {
  hal_uart_init(HAL_UART_1, &(hal_uart_config_t){.baudrate = 9600});
  volatile UARTx_Reg_Typedef *u1 = GET_USARTx_BASE(1);
  uint32_t pclk = hal_clock_get_apb2clk();
  uint32_t expected_brr = (pclk + 4800) / 9600;
  TEST_ASSERT_EQUAL_UINT32(expected_brr, u1->BRR);
}

void test_uart_baudrate_115200(void) {
  hal_uart_init(HAL_UART_6, &(hal_uart_config_t){.baudrate = 115200});
  volatile UARTx_Reg_Typedef *u6 = GET_USARTx_BASE(6);
  uint32_t pclk = hal_clock_get_apb2clk();
  uint32_t expected_brr = (pclk + 57600) / 115200;
  TEST_ASSERT_EQUAL_UINT32(expected_brr, u6->BRR);
}
