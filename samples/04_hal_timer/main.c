/**
 * @file main.c
 * @brief Example application: Print Timer5 count over HAL_UART_2 continuously.
 *
 * @details
 * - Initializes SysTick with 1 ms tick.
 * - Initializes HAL_UART_2 at 9600 baud.
 * - Initializes TIM5 timer.
 * - Continuously prints the TIM5 count via HAL_UART_2.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include <stdint.h>
#define CORTEX_M4
#include "navhal.h"
int old = 0;
void print() {
  uint32_t t = hal_timebase_get_tick();
  int r = t - old;
  old = t;
  hal_uart_print(HAL_UART_2, r);
  hal_uart_print(HAL_UART_2, "\n\r");
}

int main(void) {

  hal_timebase_init(1000);       /**< Initialize SysTick with 1 ms tick */
  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=9600});         /**< Initialize HAL_UART_2 at 9600 baud */
  hal_timer_init_freq(TIM5, 1000); /**< Initialize TIM5 with 1 Hz frequency */
  uint32_t old = 0;
  hal_timer_attach_callback(TIM5, print); /**< Newline and carriage return */
  hal_timer_enable_interrupt(TIM5);
  while (1) {
  }
}
