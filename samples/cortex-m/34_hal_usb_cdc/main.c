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
 * @brief USB CDC-ACM virtual serial port: echo what the host types.
 *
 * @details
 * The board enumerates as a virtual COM port. Open it and every character sent
 * comes back; the LED blinks once a second so an un-opened port is still
 * visibly alive.
 *
 * Open the port with local echo OFF — the default for screen/picocom/minicom.
 * A terminal that echoes received characters back forms a feedback loop with
 * any echo device and saturates the link.
 *
 * @code
 * screen /dev/ttyACM0 115200      # or: picocom, minicom — the baud is cosmetic
 * @endcode
 *
 * USB full speed needs exactly 48 MHz, so the PLL is set up first:
 * 8 MHz HSE / M=8 -> 1 MHz, * N=336 -> 336 MHz VCO, / P=4 -> 84 MHz SYSCLK,
 * / Q=7 -> 48 MHz USB clock. ::hal_usb_cdc_init refuses to run on any other
 * clock rather than enumerate unreliably.
 */

#include "board.h"
#include "navhal.h"

static hal_pll_config_t pll_cfg = {
    .input_src = HAL_CLOCK_SOURCE_HSE,
    .pll_m = 8,
    .pll_n = 336,
    .pll_p = 4,
    .pll_q = 7,
};

static hal_clock_config_t clk_cfg = {.source = HAL_CLOCK_SOURCE_PLL};

int main(void) {
  hal_clock_init(&clk_cfg, &pll_cfg);
  hal_timebase_init(1000);

  hal_gpio_set_mode(LED_BUILTIN, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
  hal_gpio_write(LED_BUILTIN, LED_OFF);

  if (hal_usb_cdc_init() != HAL_OK) {
    /* Wrong USB clock — fast-blink forever so the failure is visible without a
     * debugger. */
    while (1) {
      hal_gpio_write(LED_BUILTIN, LED_ON);
      hal_delay_ms(60);
      hal_gpio_write(LED_BUILTIN, LED_OFF);
      hal_delay_ms(60);
    }
  }

  uint8_t buf[64];

  while (1) {
    if (hal_usb_cdc_connected()) {
      uint16_t n = hal_usb_cdc_read(buf, sizeof(buf));
      if (n)
        hal_usb_cdc_write(buf, n);
    }

    /* Heartbeat off the millisecond clock: on for the first 30 ms of every
     * second. Delaying in the loop instead would cap the echo at one 64-byte
     * packet per delay. */
    uint32_t ms = hal_timebase_get_millis();
    hal_gpio_write(LED_BUILTIN, (ms % 1000u) < 30u ? LED_ON : LED_OFF);
  }
}
