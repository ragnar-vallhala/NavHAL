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
 * @brief Stage-2, as far as slice 5 needs it: prove stage-1 handed over.
 *
 * Slice 6 turns this into the real second stage -- app verification, update mode
 * over UART and CDC, the rollback floor. For now it exists so stage-1's verify
 * and jump can be tested against a signed image rather than against nothing.
 *
 * Linked at 0x08010200: the partition base plus the 512-byte header stage-1
 * checked before jumping here.
 */
#include "board.h"
#include "common/hal_boot.h"
#include "navhal.h"

int main(void) {
  /* The clock is already at 84 MHz -- stage-1 set it -- but say so rather than
   * assume: a stage-2 reached by any other route needs it too. */
  hal_clock_init_hz(HAL_CLOCK_SOURCE_HSE, 84000000u);
  hal_uart_init(BOARD_CONSOLE_UART, &(hal_uart_config_t){.baudrate = 115200});

  hal_uart_print(BOARD_CONSOLE_UART, "stage2: reached, verified by stage1\r\n");

  /* Prove VTOR moved: a tick means this image's handler ran from this image's
   * vector table, not stage-1's. */
  hal_timebase_init(1000u);
  uint32_t t0 = hal_timebase_get_millis();
  while (hal_timebase_get_millis() - t0 < 50u)
    ;
  hal_uart_print(BOARD_CONSOLE_UART, "stage2: systick runs from our vectors\r\n");

  /* The application's half of the crashloop contract: clear the strike count
   * only after proving liveness, never at startup. A stage that cleared it
   * immediately would reset its own counter every boot, and a crash a second
   * later would never be counted. */
  hal_delay_ms(2000u);
  (void)hal_boot_mark_healthy();
  hal_uart_print(BOARD_CONSOLE_UART, "stage2: healthy, strikes cleared\r\n");

  for (;;) {
    hal_uart_print(BOARD_CONSOLE_UART, "stage2: alive\r\n");
    hal_delay_ms(1000u);
  }
}
