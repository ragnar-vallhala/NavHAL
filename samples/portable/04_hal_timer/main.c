/**
 * @file main.c
 * @brief Timer-driven print of the elapsed-tick delta over the console UART.
 *
 * @details
 * Target-agnostic: the console UART and the general-purpose timer are named
 * by the board-layer aliases ::BOARD_CONSOLE_UART and ::BOARD_GP_TIMER, so
 * the same source builds for the Nucleo-F401RE and the ATmega328P.
 *
 * The general-purpose timer fires an update interrupt at 1 kHz; its callback
 * prints how many timebase ticks elapsed since the previous callback.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include <stdint.h>

#include "board.h"
#include "navhal.h"

static uint32_t old_tick = 0;

static void on_timer(void) {
  uint32_t now = hal_timebase_get_tick();
  hal_uart_print(BOARD_CONSOLE_UART, now - old_tick);
  hal_uart_print(BOARD_CONSOLE_UART, "\n\r");
  old_tick = now;
}

int main(void) {
  hal_timebase_init(1000); /* 1 ms tick */
  hal_uart_init(BOARD_CONSOLE_UART, &(hal_uart_config_t){.baudrate = 9600});
  hal_timer_init_freq(BOARD_GP_TIMER, 1000);
  hal_timer_attach_callback(BOARD_GP_TIMER, on_timer);
  hal_timer_enable_interrupt(BOARD_GP_TIMER);

  while (1) {
  }
}
