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
