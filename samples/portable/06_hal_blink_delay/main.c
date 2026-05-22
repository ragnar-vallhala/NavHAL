/**
 * @file main.c
 * @brief Blink the on-board LED and print elapsed milliseconds.
 *
 * @details
 * Target-agnostic: the LED and console UART are named by the board-layer
 * aliases ::LED_BUILTIN and ::BOARD_CONSOLE_UART, so the same source builds
 * for the Nucleo-F401RE and the ATmega328P.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
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
