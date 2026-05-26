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
 * @file main.cpp
 * @brief C++ blink sample — proves NavHAL headers work from a C++ TU.
 *
 * @details
 * Mirrors samples/portable/01_hal_blink in behaviour but written in C++17.
 * Exercises the project-wide `extern "C"` guards: a tiny `Pin` wrapper (C++)
 * calls standardized `hal_gpio_*` functions (C linkage) through the umbrella
 * navhal.h. If linkage were broken, the linker would fail to resolve the
 * mangled C++ calls against the C-compiled HAL archive.
 *
 * Target-agnostic: the LED and console UART are named by the board-layer
 * aliases ::LED_BUILTIN and ::BOARD_CONSOLE_UART, so the same source builds
 * for the Nucleo-F401RE and the ATmega328P.
 *
 * Constraints (bare-metal): built with -fno-exceptions / -fno-rtti, and no
 * globals with non-trivial constructors — `Pin` is a `constexpr` literal.
 */

#include "board.h"
#include "navhal.h"

namespace {

class Pin {
public:
  constexpr explicit Pin(hal_gpio_pin_t id) noexcept : id_(id) {}

  void as_output() const {
    hal_gpio_enable_clock(id_);
    hal_gpio_set_mode(id_, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
  }
  void high() const { hal_gpio_write(id_, HAL_GPIO_HIGH); }
  void low() const { hal_gpio_write(id_, HAL_GPIO_LOW); }
  void toggle() const { hal_gpio_toggle(id_); }

private:
  hal_gpio_pin_t id_;
};

} // namespace

int main() {
  hal_timebase_init(1000);

  hal_uart_config_t uart_cfg{};
  uart_cfg.baudrate = 9600;
  hal_uart_init(BOARD_CONSOLE_UART, &uart_cfg);

  const Pin led{LED_BUILTIN};
  led.as_output();

  hal_uart_write_string(BOARD_CONSOLE_UART,
                        "NavHAL C++17 blink sample\r\n");

  while (true) {
    led.toggle();
    hal_delay_ms(500);
  }
}
