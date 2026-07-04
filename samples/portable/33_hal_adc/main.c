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
 * @brief Sample one analog input and print the raw ADC code every 500 ms.
 *
 * @details
 * Target-agnostic: the console UART and the analog input (unit, pin, channel)
 * are named by the board-layer aliases (::BOARD_CONSOLE_UART, ::BOARD_ADC,
 * ::BOARD_ADC_PIN, ::BOARD_ADC_CHANNEL), so the same source builds and runs on
 * the Nucleo-F401RE, the Nucleo-F767ZI, and the ATmega328P. The printed code is
 * raw and right-aligned: 0..4095 on the STM32 parts (12-bit), 0..1023 on the
 * ATmega328P (10-bit).
 */

#include "board.h"
#include "navhal.h"

int main(void) {
  hal_timebase_init(1000);
  hal_uart_init(BOARD_CONSOLE_UART, &(hal_uart_config_t){.baudrate = 9600});
  hal_uart_print(BOARD_CONSOLE_UART, "HAL ADC demo — sampling A0.\n\r");

  /* Put the analog pin in analog mode (a harmless no-op on the ATmega). */
  hal_gpio_enable_clock(BOARD_ADC_PIN);
  hal_gpio_set_mode(BOARD_ADC_PIN, HAL_GPIO_MODE_ANALOG, HAL_GPIO_PULL_NONE);

  if (hal_adc_init(BOARD_ADC, NULL) != HAL_OK) {
    hal_uart_print(BOARD_CONSOLE_UART, "ADC init failed.\n\r");
    while (1)
      ;
  }

  while (1) {
    uint16_t raw = 0;
    if (hal_adc_read(BOARD_ADC, BOARD_ADC_CHANNEL, &raw) == HAL_OK) {
      hal_uart_print(BOARD_CONSOLE_UART, "ADC A0 = ");
      hal_uart_write_int(BOARD_CONSOLE_UART, (int)raw);
      hal_uart_print(BOARD_CONSOLE_UART, "\n\r");
    } else {
      hal_uart_print(BOARD_CONSOLE_UART, "ADC read timeout.\n\r");
    }
    hal_delay_ms(500);
  }
}
