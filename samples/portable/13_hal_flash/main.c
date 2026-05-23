/**
 * @file main.c
 * @brief Flash key/value store demo — save a record, read it back, print it.
 *
 * @details
 * Target-agnostic via ::BOARD_CONSOLE_UART. The key/value store is backed by
 * on-chip flash on Cortex-M and by EEPROM on the ATmega328P.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
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
