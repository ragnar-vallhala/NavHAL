/**
 * @file main.c
 * @brief Send "Hello World" over the board console UART (polling TX).
 *
 * @details
 * Target-agnostic: the UART is named by the board-layer alias
 * ::BOARD_CONSOLE_UART, so the same source builds for the Nucleo-F401RE
 * (USART2 / ST-LINK VCP) and for the ATmega328P (USART0) with only a
 * Kconfig target switch.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
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
