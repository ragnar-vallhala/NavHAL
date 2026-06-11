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
 * @brief Flash key/value store demo — save a record, read it back, print it.
 *
 * @details
 * Target-agnostic via ::BOARD_CONSOLE_UART. The key/value store is backed by
 * on-chip flash on Cortex-M and by EEPROM on the ATmega328P.
 */

#include "board.h"
#include "navhal.h"

hal_flash_record_t rec;

int main(void) {
  hal_timebase_init(1000);
  hal_uart_init(BOARD_CONSOLE_UART, &(hal_uart_config_t){.baudrate = 9600});
  hal_uart_print(BOARD_CONSOLE_UART, "HAL Flash Sample Application\n");

  rec.crc = 123;
  hal_flash_save(1, (const uint8_t *)&rec, sizeof(hal_flash_record_t));

  uint8_t size = sizeof(hal_flash_record_t);
  hal_flash_record_t buff;
  hal_flash_read(1, (uint8_t *)&buff, &size);

  hal_uart_print(BOARD_CONSOLE_UART, "rec CRC: ");
  hal_uart_print(BOARD_CONSOLE_UART, rec.crc);
  hal_uart_print(BOARD_CONSOLE_UART, " | Key: ");
  hal_uart_print(BOARD_CONSOLE_UART, rec.key);
  hal_uart_print(BOARD_CONSOLE_UART, " | Magic: ");
  hal_uart_print(BOARD_CONSOLE_UART, rec.magic);
  hal_uart_print(BOARD_CONSOLE_UART, " | Reserved: ");
  hal_uart_print(BOARD_CONSOLE_UART, rec.reserved);
  hal_uart_print(BOARD_CONSOLE_UART, " | Size: ");
  hal_uart_print(BOARD_CONSOLE_UART, rec.size);
  hal_uart_print(BOARD_CONSOLE_UART, " | Status: ");
  hal_uart_print(BOARD_CONSOLE_UART, rec.status);
  hal_uart_print(BOARD_CONSOLE_UART, " \n");

  hal_uart_print(BOARD_CONSOLE_UART, "buff CRC: ");
  hal_uart_print(BOARD_CONSOLE_UART, buff.crc);
  hal_uart_print(BOARD_CONSOLE_UART, " | Key: ");
  hal_uart_print(BOARD_CONSOLE_UART, buff.key);
  hal_uart_print(BOARD_CONSOLE_UART, " | Magic: ");
  hal_uart_print(BOARD_CONSOLE_UART, buff.magic);
  hal_uart_print(BOARD_CONSOLE_UART, " | Reserved: ");
  hal_uart_print(BOARD_CONSOLE_UART, buff.reserved);
  hal_uart_print(BOARD_CONSOLE_UART, " | Size: ");
  hal_uart_print(BOARD_CONSOLE_UART, buff.size);
  hal_uart_print(BOARD_CONSOLE_UART, " | Status: ");
  hal_uart_print(BOARD_CONSOLE_UART, buff.status);
  hal_uart_print(BOARD_CONSOLE_UART, " \n");

  hal_uart_print(BOARD_CONSOLE_UART, "Storage needs compaction: ");
  hal_uart_print(BOARD_CONSOLE_UART, hal_flash_needs_compaction() ? 1 : 0);
  hal_uart_print(BOARD_CONSOLE_UART, " \n");

  while (1) {
  }
}
