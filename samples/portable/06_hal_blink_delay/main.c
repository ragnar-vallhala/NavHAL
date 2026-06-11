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
 * @brief Blink the on-board LED and print elapsed milliseconds.
 *
 * @details
 * Target-agnostic: the LED and console UART are named by the board-layer
 * aliases ::LED_BUILTIN and ::BOARD_CONSOLE_UART, so the same source builds
 * for the Nucleo-F401RE and the ATmega328P.
 */

#include "board.h"
#include "navhal.h"

int main(void) {
  hal_timebase_init(40); /* 40 us tick */
  hal_uart_init(BOARD_CONSOLE_UART, &(hal_uart_config_t){.baudrate = 9600});
  hal_gpio_set_mode(LED_BUILTIN, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);

  while (1) {
    hal_gpio_write(LED_BUILTIN, HAL_GPIO_HIGH);
    hal_delay_ms(500);
    hal_gpio_write(LED_BUILTIN, HAL_GPIO_LOW);
    hal_delay_ms(500);

    hal_uart_print(BOARD_CONSOLE_UART, "Millis: ");
    hal_uart_print(BOARD_CONSOLE_UART, hal_timebase_get_millis());
    hal_uart_print(BOARD_CONSOLE_UART, "\n\r");
  }
}
