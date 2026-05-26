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

#define CORTEX_M4
#include "navhal.h"

int main() {
  hal_timebase_init(1000); /**< Initialize SysTick for 1ms ticks */
  hal_uart_init(HAL_UART_6, &(hal_uart_config_t){.baudrate=9600});

  /* --- DMA benchmark --- */
#if defined(_DMA_ENABLED) && defined(_UART_BACKEND_DMA)
  int n = hal_timebase_get_tick();
  int iter = 100;
  while (iter--)
    hal_uart_write_string_dma(HAL_UART_6, "Hello World\n\r"); /**< DMA transfer */
  hal_uart_write_string(HAL_UART_6, "DMA done: ");
  hal_uart_print(HAL_UART_6, hal_timebase_get_tick() - n);
  hal_uart_write_string(HAL_UART_6, " ticks\n\r");
#else
  /* --- Polling benchmark (fallback) --- */
  int n = hal_timebase_get_tick();
  int iter = 100;
  while (iter--)
    hal_uart_write_string(HAL_UART_2, "Hello World\n\r"); /**< Polling transfer */
  hal_uart_write_string(HAL_UART_2, "Poll done: ");
  hal_uart_print(HAL_UART_2, hal_timebase_get_tick() - n);
  hal_uart_write_string(HAL_UART_2, " ticks\n\r");
#endif
  return 0;
}