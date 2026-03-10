/**
 * @file main.c
 * @brief Example application: Print Timer5 count over UART2 continuously.
 *
 * @details
 * - Initializes SysTick with 1 ms tick.
 * - Initializes UART2 at 9600 baud.
 * - Initializes TIM5 timer.
 * - Continuously prints the TIM5 count via UART2.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include <stdint.h>
#define CORTEX_M4
#include "navhal.h"
int old = 0;
void print() {
  uint32_t t = hal_get_tick();
  int r = t - old;
  old = t;
  uart2_write(r);
  uart2_write("\n\r");
}

int main(void) {

  systick_init(1000);       /**< Initialize SysTick with 1 ms tick */
  uart2_init(9600);         /**< Initialize UART2 at 9600 baud */
  timer_init_freq(TIM5, 1000); /**< Initialize TIM5 with 1 Hz frequency */
  uint32_t old = 0;
  timer_attach_callback(TIM5, print); /**< Newline and carriage return */
  timer_enable_interrupt(TIM5);
  while (1) {
  }
}
