/**
 * @file main.cpp
 * @brief C++ blink sample — proves NavHAL headers work from a C++ TU.
 *
 * Mirrors samples/01_hal_blink in behavior but written in C++17. Exercises
 * the project-wide `extern "C"` guards: a tiny `Pin` wrapper (C++) calls
 * standardized `hal_gpio_*` functions (C linkage) through the umbrella
 * navhal.h. If linkage were broken, the linker would fail to resolve the
 * mangled C++ calls against the C-compiled HAL archive.
 *
 * Uses the standardized hal_* surface throughout — the legacy aliases
 * under include/compat expand to `_Generic` for things like uart2_write,
 * which is a C11 construct with no C++ equivalent. Idiomatic C++ code
 * targets the typed entry points directly.
 *
 * Constraints (bare-metal, -nostdlib):
 *   - No exceptions (-fno-exceptions) and no RTTI (-fno-rtti).
 *   - No globals with non-trivial constructors — the startup code does
 *     not iterate .init_array, so any non-constexpr global ctor would
 *     simply never run.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#define CORTEX_M4
#include "navhal.h"

namespace {

class Pin {
public:
  constexpr explicit Pin(hal_gpio_pin_t id) noexcept : id_(id) {}

  void as_output() const {
    hal_gpio_enable_clock(id_);
    hal_gpio_set_mode(id_, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
  }
  void high()   const { hal_gpio_write(id_, HAL_GPIO_HIGH); }
  void low()    const { hal_gpio_write(id_, HAL_GPIO_LOW); }
  void toggle() const { hal_gpio_toggle(id_); }

private:
  hal_gpio_pin_t id_;
};

} // namespace

int main() {
  hal_timebase_init(1000);

  hal_uart_config_t uart_cfg{};
  uart_cfg.baudrate = 9600;
  hal_uart_init(HAL_UART_2, &uart_cfg);

  constexpr Pin led{GPIO_PA05};
  led.as_output();

  hal_uart_write_string(HAL_UART_2,
                        "NavHAL C++17 blink sample on PA05\r\n");

  while (true) {
    led.toggle();
    hal_delay_ms(500);
  }
}
