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
 * @brief Send "Hello World" over the board console UART (polling TX).
 *
 * @details
 * Target-agnostic: the UART is named by the board-layer alias
 * ::BOARD_CONSOLE_UART, so the same source builds for the Nucleo-F401RE
 * (USART2 / ST-LINK VCP) and for the ATmega328P (USART0) with only a
 * Kconfig target switch.
 */

#include "board.h"
#include "navhal.h"

int main(void) {
  hal_timebase_init(1000);
  hal_uart_init(BOARD_CONSOLE_UART, &(hal_uart_config_t){.baudrate = 115200});

  uint32_t start = hal_timebase_get_tick();
  int iter = 100;
  while (iter--) {
    hal_uart_print(BOARD_CONSOLE_UART, "Hello World\n\r");
  }

  hal_uart_print(BOARD_CONSOLE_UART, "UART TX NO DMA Test finished: ");
  hal_uart_print(BOARD_CONSOLE_UART, hal_timebase_get_tick() - start);
  hal_uart_print(BOARD_CONSOLE_UART, " ticks\n\r");
  return 0;
}
