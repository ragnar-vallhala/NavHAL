/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
