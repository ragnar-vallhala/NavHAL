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
 * @brief PWM duty-cycle ramp, with the duty printed over the console UART.
 *
 * @details
 * Target-agnostic: the console UART and the PWM timer / channel / pin are
 * named by the board-layer aliases (::BOARD_CONSOLE_UART, ::BOARD_PWM_TIMER,
 * ::BOARD_PWM_CHANNEL, ::BOARD_PWM_PIN), so the same source builds for the
 * Nucleo-F401RE and the ATmega328P.
 *
 * The duty cycle ramps 0 % -> 100 % in 1 % steps every 10 ms.
 */

#include "board.h"
#include "navhal.h"

int main(void) {
  hal_timebase_init(40);
  hal_uart_init(BOARD_CONSOLE_UART, &(hal_uart_config_t){.baudrate = 9600});

  /* On Cortex-M the timer output is routed through a GPIO alternate
   * function; on the AVR the PWM driver owns the fixed OCnx pin, so these
   * two calls are harmless no-ops there. */
  hal_gpio_set_mode(BOARD_PWM_PIN, HAL_GPIO_MODE_AF, HAL_GPIO_PULL_NONE);
  hal_gpio_set_alternate_function(BOARD_PWM_PIN, HAL_GPIO_AF1);

  hal_pwm_handle_t pwm = {BOARD_PWM_TIMER, BOARD_PWM_CHANNEL};
  hal_pwm_init(&pwm, 15000, 0.10f);
  hal_pwm_start(&pwm);

  float value = 0.0f;
  while (1) {
    hal_pwm_set_duty_cycle(&pwm, value);
    value += 0.01f;
    if (value >= 1.0f)
      value = 0.0f;

    hal_uart_print(BOARD_CONSOLE_UART, value);
    hal_uart_print(BOARD_CONSOLE_UART, "\n\r");
    hal_delay_ms(10);
  }
}
