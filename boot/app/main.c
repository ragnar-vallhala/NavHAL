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
 * @brief A minimal application, as far as the bootloader needs one.
 *
 * Linked at 0x08020200 behind its own signed header, reached only because
 * stage-2 verified it and found its version at or above the rollback floor.
 *
 * It plays the application's half of the crashloop contract: clear the strike
 * count once it has proven it is alive, never at startup. Slice 7 adds the real
 * article -- the sniffer on both transports and the entry policy gate.
 */
#include "board.h"
#include "common/hal_boot.h"
#include "navhal.h"

int main(void) {
  hal_clock_init_hz(HAL_CLOCK_SOURCE_HSE, 84000000u);
  hal_timebase_init(1000u);
  hal_uart_init(BOARD_CONSOLE_UART, &(hal_uart_config_t){.baudrate = 115200});

  hal_uart_print(BOARD_CONSOLE_UART, "app: running, verified by stage2\r\n");

  /* The watchdog is already running: stage-2 started it and the IWDG cannot be
   * stopped, only re-periodised. An application that ignores it is reset on the
   * loader's timeout -- which is how this was found, with the board rebooting
   * every eight seconds while printing happily. Adopting it sets a period that
   * suits the application rather than the loader. */
  (void)hal_watchdog_start(2000u);

  /* Liveness first. An application that cleared its strikes at startup would
   * reset the counter every boot, and a fault one second later would never be
   * counted -- the crashloop would be invisible to the thing meant to catch it. */
  for (int i = 0; i < 30; i++) { /* 3 s, kicking as it goes */
    hal_delay_ms(100u);
    (void)hal_watchdog_kick();
  }
  (void)hal_boot_mark_healthy();
  hal_uart_print(BOARD_CONSOLE_UART, "app: healthy, strikes cleared\r\n");

  for (;;) {
    hal_uart_print(BOARD_CONSOLE_UART, "app: alive\r\n");
    for (int i = 0; i < 10; i++) {
      hal_delay_ms(100u);
      (void)hal_watchdog_kick();
    }
  }
}
