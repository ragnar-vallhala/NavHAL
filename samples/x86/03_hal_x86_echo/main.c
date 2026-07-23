/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file main.c
 * @brief x86-64 UART RX demo: read a line from COM1 and echo it back.
 *
 * Under QEMU, `-serial stdio` wires COM1 to the host terminal. That terminal is
 * in raw mode: it does NOT echo keystrokes and it sends '\r' (not '\n') on
 * Enter. So this reads character by character, echoes each one so you can see
 * what you type, and ends the line on either '\r' or '\n'.
 */

#include "board.h"
#include "common/hal_uart.h"

int main(void) {
  hal_uart_config_t cfg = {.baudrate = 115200};
  hal_uart_init(CONSOLE_UART, &cfg);
  hal_uart_write_string(CONSOLE_UART, "echo ready - type a line:\r\n> ");

  char line[64];
  uint32_t n = 0;
  for (;;) {
    char c = hal_uart_read_char(CONSOLE_UART);
    if (c == '\r' || c == '\n') {
      line[n] = '\0';
      hal_uart_write_string(CONSOLE_UART, "\r\nyou said (");
      hal_uart_write_uint(CONSOLE_UART, n);
      hal_uart_write_string(CONSOLE_UART, "): ");
      hal_uart_write_string(CONSOLE_UART, line);
      hal_uart_write_string(CONSOLE_UART, "\r\n> ");
      n = 0;
    } else if (c == '\b' || c == 0x7F) { /* backspace / delete */
      if (n > 0) {
        n--;
        hal_uart_write_string(CONSOLE_UART, "\b \b");
      }
    } else if (n < sizeof(line) - 1) {
      line[n++] = c;
      hal_uart_write_char(CONSOLE_UART, c); /* live echo */
    }
  }
}
