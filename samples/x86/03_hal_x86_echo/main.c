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
 * Under QEMU, `-serial stdio` wires COM1 to the host terminal, so lines you type
 * (or pipe in) are read via hal_uart_read_until and echoed with their length.
 */

#include "board.h"
#include "common/hal_uart.h"

int main(void) {
  hal_uart_config_t cfg = {.baudrate = 115200};
  hal_uart_init(CONSOLE_UART, &cfg);
  hal_uart_write_string(CONSOLE_UART, "echo ready - type a line:\r\n");

  char line[64];
  for (;;) {
    uint32_t n = hal_uart_read_until(CONSOLE_UART, line, sizeof line, '\n');
    hal_uart_write_string(CONSOLE_UART, "you said (");
    hal_uart_write_uint(CONSOLE_UART, n);
    hal_uart_write_string(CONSOLE_UART, "): ");
    hal_uart_write_string(CONSOLE_UART, line);
    hal_uart_write_string(CONSOLE_UART, "\r\n");
  }
}
