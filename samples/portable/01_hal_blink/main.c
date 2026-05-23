/**
 * @file main.c
 * @brief Blink the on-board LED using the portable NavHAL API.
 *
 * @details
 * Target-agnostic: the LED is named by the board-layer alias
 * ::LED_BUILTIN, so the same source builds for the Nucleo-F401RE and for
 * the ATmega328P with only a Kconfig target switch.
 *
 * - Initializes the timebase with a 1 ms tick.
 * - Configures ::LED_BUILTIN as an output.
 * - Toggles it with a 100 ms delay in an infinite loop.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "board.h"
#include "navhal.h"

int main(void) {
  hal_timebase_init(1000);
  hal_gpio_set_mode(LED_BUILTIN, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);

  while (1) {
    hal_gpio_write(LED_BUILTIN, HAL_GPIO_HIGH);
    hal_delay_ms(100);
    hal_gpio_write(LED_BUILTIN, HAL_GPIO_LOW);
    hal_delay_ms(100);
  }
}
