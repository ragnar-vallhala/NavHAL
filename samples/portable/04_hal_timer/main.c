/**
 * @file main.c
 * @brief Timer-driven print of the elapsed-tick delta over the console UART.
 *
 * @details
 * Target-agnostic: the console UART and the general-purpose timer are named
 * by the board-layer aliases ::BOARD_CONSOLE_UART and ::BOARD_GP_TIMER, so
 * the same source builds for the Nucleo-F401RE and the ATmega328P.
 *
 * The general-purpose timer fires an update interrupt at 1 Hz; its callback
 * prints how many timebase ticks elapsed since the previous callback (~1000 —
 * the 1 ms timebase ticks in one second). The callback does blocking UART
 * output, so the timer is kept slow enough that it finishes within one
 * period: an AVR ISR is non-preemptible, and a too-fast timer would starve
 * the timebase ISR and the delta would read 0.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include <stdint.h>

#include "board.h"
#include "navhal.h"

static uint32_t old_tick = 0;
/* Written by the timer ISR, polled by main() — must be volatile so the
 * compiler re-reads it from memory each loop iteration. */
static volatile int32_t gap = -1;
static void on_timer(void) {
  uint32_t now = hal_timebase_get_tick();
  gap = now - old_tick;
  old_tick = now;
}

int main(void) {
  hal_timebase_init(1000); /* 1 ms tick */
  hal_uart_init(BOARD_CONSOLE_UART, &(hal_uart_config_t){.baudrate = 9600});
  hal_timer_init_freq(BOARD_GP_TIMER, 1);
  hal_timer_attach_callback(BOARD_GP_TIMER, on_timer);
  hal_timer_enable_interrupt(BOARD_GP_TIMER);

  while (1) {
    if (gap > -1) {
      hal_uart_print(BOARD_CONSOLE_UART, gap);
      hal_uart_print(BOARD_CONSOLE_UART, "\n\r");
      gap = -1;
    }
  }
}
