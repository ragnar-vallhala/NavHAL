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
 * It plays the application's three parts of the contract, which no loader can
 * play for it: clear the strike count once it has proven it is alive, watch both
 * links for the sequence that asks for the loader, and refuse that request while
 * it would be dangerous to honour.
 */
#include "board.h"
#include "common/hal_boot.h"
#include "navhal.h"

#define LIVENESS_MS 3000u
#define APP_WATCHDOG_MS 2000u

static void say(const char *s) { hal_uart_print(BOARD_CONSOLE_UART, s); }

#if NAVHAL_CONFIG_DRV_USB_CDC
/* Forwarding rather than registering hal_boot_feed directly: the callback takes
 * the stream over instead of tapping it, so an application that registers the
 * matcher as its own callback can never read CDC again. */
static void cdc_rx(const uint8_t *data, uint16_t len) {
  hal_boot_feed(data, len);
  /* An application with a CDC protocol of its own would consume data here too. */
}
#endif

/* Kick while waiting, so a delay is never what trips the watchdog. */
static void wait_ms(uint32_t ms) {
  for (uint32_t i = 0; i < ms; i += 50u) {
    hal_delay_ms(50u);
    (void)hal_watchdog_kick();
  }
}

int main(void) {
  hal_clock_init_hz(HAL_CLOCK_SOURCE_HSE, 84000000u);
  hal_timebase_init(1000u);
  hal_uart_init(BOARD_CONSOLE_UART, &(hal_uart_config_t){.baudrate = 115200});

  /* The watchdog is already running: stage-2 started it and the IWDG cannot be
   * stopped, only re-periodised. An application that ignores it is reset on the
   * loader's timeout -- which is how this was found, with the board rebooting
   * every eight seconds while printing happily. */
  (void)hal_watchdog_start(APP_WATCHDOG_MS);

  say("app: running, verified by stage2\r\n");

#if NAVHAL_CONFIG_DRV_USB_CDC
  /* Stage-2 may hand over an already enumerated peripheral; init is a no-op
   * then. Watching both links matters because the one an operator has is not
   * always the one the board was flashed through. */
  (void)hal_usb_cdc_init();
  (void)hal_usb_cdc_set_rx_callback(cdc_rx);
#endif

  /* Liveness first. An application that cleared its strikes at startup would
   * reset the counter every boot, and a fault one second later would never be
   * counted -- the crashloop would be invisible to the thing meant to catch it.
   * Up to here a reset leaves the strike standing, which is the point. */
  say("app: proving liveness\r\n");
  wait_ms(LIVENESS_MS);
  (void)hal_boot_mark_healthy();
  say("app: healthy, strikes cleared\r\n");

  say("app: watching both links for the loader sequence\r\n");

  bool armed = false;
  for (;;) {
    (void)hal_watchdog_kick();

    while (hal_uart_available(BOARD_CONSOLE_UART)) {
      uint8_t b = (uint8_t)hal_uart_read_char(BOARD_CONSOLE_UART);
      /* 'a' and 'd' stand in for an airframe arming and disarming. A real
       * vehicle calls these from the arming path, not from its console. */
      if (b == 'a') {
        armed = true;
        hal_boot_entry_disable();
        say("app: armed -- loader entry refused\r\n");
      } else if (b == 'd') {
        armed = false;
        hal_boot_entry_enable();
        say("app: disarmed -- loader entry allowed\r\n");
      } else {
        /* Does not return if it completes the sequence and entry is allowed. */
        hal_boot_match_byte(b);
      }
    }

    static uint32_t last = 0;
    uint32_t now = hal_timebase_get_millis();
    if (now - last >= 2000u) {
      last = now;
      say(armed ? "app: alive (armed)\r\n" : "app: alive\r\n");
    }
  }
}
