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
 * @brief Let a watchdog reset the board, then read back that it did.
 *
 * @details
 * The two halves of the API only mean anything together, so this exercises
 * both: arm the independent watchdog, kick it for a few seconds to show the
 * board staying alive, then stop kicking and let it bite. The next boot reads
 * ::HAL_RESET_CAUSE_WATCHDOG out of ::hal_reset_get_cause and says so — which
 * is the whole point, since a device that recovers silently is
 * indistinguishable from one that never failed.
 *
 * Expected output on the console UART, looping every cycle:
 * @code
 * --- boot, reset cause: power-on
 * watchdog armed for 2000 ms
 * kick 1 ... kick 5
 * starving the watchdog now
 * --- boot, reset cause: watchdog
 * @endcode
 *
 * Target-agnostic: the console is ::BOARD_CONSOLE_UART, so the same source
 * builds for the Nucleo boards and the ATmega328P.
 *
 * @note On an Arduino Uno the cause prints as `unknown` even though the reset
 * really was the watchdog. The bootloader reads MCUSR and clears it before the
 * application starts — it has to, or a watchdog reset would loop forever — and
 * this one keeps no copy anywhere the application can reach. The watchdog half
 * still works and the board still cycles; it is only the reporting that the
 * bootloader has taken away. A bare chip programmed over ISP reports properly.
 */

#include "board.h"
#include "navhal.h"

#define WATCHDOG_MS 2000U
#define KICKS_BEFORE_STARVING 5U

static void print(const char *s) { hal_uart_print(BOARD_CONSOLE_UART, s); }

static void print_uint(uint32_t v) {
  char buf[12];
  int i = 11;
  buf[i] = '\0';
  if (v == 0U)
    buf[--i] = '0';
  while (v && i > 0) {
    buf[--i] = (char)('0' + (v % 10U));
    v /= 10U;
  }
  print(&buf[i]);
}

/** @brief Name the first cause bit set, which is enough for a log line. */
static const char *cause_name(uint32_t cause) {
  if (cause & HAL_RESET_CAUSE_WATCHDOG)
    return "watchdog";
  if (cause & HAL_RESET_CAUSE_WINDOW_WATCHDOG)
    return "window watchdog";
  if (cause & HAL_RESET_CAUSE_SOFTWARE)
    return "software";
  if (cause & HAL_RESET_CAUSE_BROWNOUT)
    return "brown-out";
  if (cause & HAL_RESET_CAUSE_PIN)
    return "pin";
  if (cause & HAL_RESET_CAUSE_POWER_ON)
    return "power-on";
  if (cause & HAL_RESET_CAUSE_LOW_POWER)
    return "low-power entry";
  return "unknown";
}

int main(void) {
  /* Early and before anything else can reset the part: the flags are sticky,
   * and this is what latches them for everyone who asks later. */
  hal_reset_init();

  hal_uart_init(BOARD_CONSOLE_UART, &(hal_uart_config_t){.baudrate = 9600});

  print("\r\n--- boot, reset cause: ");
  print(cause_name(hal_reset_get_cause()));
  print("\r\n");

  if (hal_watchdog_start(WATCHDOG_MS) != HAL_OK) {
    print("watchdog refused the requested timeout\r\n");
    for (;;) {
    }
  }

  print("watchdog armed for ");
  print_uint(hal_watchdog_get_timeout_ms()); /* what rounding actually gave us */
  print(" ms\r\n");

  /* Kick at a third of the period. The oscillator behind the watchdog is an RC
   * that drifts several percent with temperature, so kicking at nearly the
   * full timeout is how a working board resets itself in the cold. */
  const uint32_t kick_ms = hal_watchdog_get_timeout_ms() / 3U;

  for (uint32_t i = 1U; i <= KICKS_BEFORE_STARVING; i++) {
    hal_delay_ms(kick_ms);
    hal_watchdog_kick();
    print("kick ");
    print_uint(i);
    print("\r\n");
  }

  print("starving the watchdog now\r\n");
  for (;;) {
    /* No kick. The reset arrives about one timeout from here, and the next
       boot prints "reset cause: watchdog". */
  }
}
