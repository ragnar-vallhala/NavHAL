/**
 * @file main.c
 * @brief CRC-32 demo — single-shot and chunked computation.
 *
 * @details
 * Target-agnostic via ::BOARD_CONSOLE_UART. CRC-32/MPEG-2 is computed by the
 * STM32F4 hardware unit on Cortex-M and by a software implementation on the
 * ATmega328P; both yield the same result, so the PASS/FAIL checks hold on
 * either target.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include <stdint.h>

#include "board.h"
#include "navhal.h"

/** @brief Print a 32-bit value as 0x-prefixed hex over the console UART. */
static void write_hex32(uint32_t val) {
  const char hex_digits[] = "0123456789ABCDEF";
  hal_uart_print(BOARD_CONSOLE_UART, "0x");
  for (int i = 28; i >= 0; i -= 4)
    hal_uart_write_char(BOARD_CONSOLE_UART, hex_digits[(val >> i) & 0xF]);
}

int main(void) {
  hal_uart_init(BOARD_CONSOLE_UART, &(hal_uart_config_t){.baudrate = 9600});

  hal_uart_print(BOARD_CONSOLE_UART,
                 "\r\n================================\r\n");
  hal_uart_print(BOARD_CONSOLE_UART, "NavHAL CRC Module Example\r\n");
  hal_uart_print(BOARD_CONSOLE_UART,
                 "================================\r\n");

  hal_crc_config_t crc_cfg = {.polynomial = HAL_CRC_POLY_CRC32,
                              .init_value = 0xFFFFFFFF};
#ifdef _CRC_HW_ENABLED
  hal_uart_print(BOARD_CONSOLE_UART, "Mode: Hardware Accelerated\r\n\r\n");
#else
  hal_uart_print(BOARD_CONSOLE_UART, "Mode: Software Fallback\r\n\r\n");
#endif
  hal_crc_init(&crc_cfg);

  /* Single-shot. */
  hal_uart_print(BOARD_CONSOLE_UART, "1. Single-Shot Computation\r\n");
  const uint8_t message[] = "123456789";
  hal_uart_print(BOARD_CONSOLE_UART, "   Message: '123456789'\r\n");
  uint32_t single_crc = hal_crc_compute(message, 9);
  hal_uart_print(BOARD_CONSOLE_UART, "   CRC32 Result:   ");
  write_hex32(single_crc);
  hal_uart_print(BOARD_CONSOLE_UART, "\r\n   Expected:       0x0376E6E7\r\n");
  hal_uart_print(BOARD_CONSOLE_UART,
                 single_crc == 0x0376E6E7 ? "   Status:         PASS\r\n\r\n"
                                          : "   Status:         FAIL\r\n\r\n");

  /* Incremental / chunked — must match the single-shot result. */
  hal_uart_print(BOARD_CONSOLE_UART, "2. Incremental Computation (Chunked)\r\n");
  const uint8_t chunk1[] = "1234";
  const uint8_t chunk2[] = "567";
  const uint8_t chunk3[] = "89";
  hal_crc_reset();
  uint32_t part1 = hal_crc_accumulate(chunk1, 4);
  uint32_t part2 = hal_crc_accumulate(chunk2, 3);
  uint32_t final_crc = hal_crc_accumulate(chunk3, 2);
  hal_uart_print(BOARD_CONSOLE_UART,
                 "   Parts appended: '1234', '567', '89'\r\n");
  hal_uart_print(BOARD_CONSOLE_UART, "   Intermediate 1: ");
  write_hex32(part1);
  hal_uart_print(BOARD_CONSOLE_UART, "\r\n   Intermediate 2: ");
  write_hex32(part2);
  hal_uart_print(BOARD_CONSOLE_UART, "\r\n   Final CRC32:    ");
  write_hex32(final_crc);
  hal_uart_print(BOARD_CONSOLE_UART, "\r\n");
  hal_uart_print(BOARD_CONSOLE_UART,
                 final_crc == 0x0376E6E7 ? "   Status:         PASS\r\n\r\n"
                                         : "   Status:         FAIL\r\n\r\n");

  hal_uart_print(BOARD_CONSOLE_UART, "CRC Example Finished. Halting.\r\n");
  while (1) {
  }
  return 0;
}
