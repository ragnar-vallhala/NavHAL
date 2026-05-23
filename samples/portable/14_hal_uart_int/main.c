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
 *
 * @copyright © NAVROBOTEC PVT. LTD.
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
