/**
 * @file main.c
 * @brief Mirror a push-button onto the on-board LED.
 *
 * @details
 * Target-agnostic: the LED and the button are named by the board-layer
 * aliases ::LED_BUILTIN and ::USER_BUTTON, so the same source builds for
 * the Nucleo-F401RE and the ATmega328P.
 *
 * The button input uses an internal pull-up; the LED is lit while the
 * button is pressed (reads low).
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "board.h"
#include "navhal.h"

int main(void) {
  hal_timebase_init(1000);
  hal_gpio_set_mode(LED_BUILTIN, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
  hal_gpio_set_mode(USER_BUTTON, HAL_GPIO_MODE_INPUT, HAL_GPIO_PULL_UP);

  while (1) {
    if (hal_gpio_read(USER_BUTTON))
      hal_gpio_write(LED_BUILTIN, HAL_GPIO_LOW);
    else
      hal_gpio_write(LED_BUILTIN, HAL_GPIO_HIGH);
  }
}
