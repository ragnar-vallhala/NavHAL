/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file main.c
 * @brief Interrupt-driven UART echo.
 *
 * @details
 * Target-agnostic: the console UART and its receive IRQ are named by the
 * board-layer aliases ::BOARD_CONSOLE_UART and ::BOARD_CONSOLE_UART_IRQ, so
 * the same source builds for the Nucleo-F401RE and the ATmega328P.
 *
 * Each received byte is echoed back from the receive-interrupt callback.
 */

#include "board.h"
#include "navhal.h"

static void uart_rx_handler(void) {
  hal_uart_write_char(BOARD_CONSOLE_UART,
                      hal_uart_read_char(BOARD_CONSOLE_UART));
}

int main(void) {
  hal_timebase_init(1000);
  hal_uart_init(BOARD_CONSOLE_UART, &(hal_uart_config_t){.baudrate = 9600});

  hal_interrupt_attach_callback(BOARD_CONSOLE_UART_IRQ, uart_rx_handler);
  hal_uart_enable_interrupt(BOARD_CONSOLE_UART, 1, 0); /* peripheral RX int */
  hal_interrupt_enable(BOARD_CONSOLE_UART_IRQ); /* NVIC line; no-op on AVR */

  while (1)
    ;
}
