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
 * @brief Stage-1: verify stage-2, or go to recovery. Nothing else.
 *
 * The root of trust. Write-protected and never erased in the field, so every
 * byte here is permanent and the list of things it does is deliberately short:
 *
 *   seed the boot block if a cold boot lost it
 *   adopt the watchdog the application left running
 *   decide: a loader request, a crashloop, or a bad signature -> recovery
 *   count the attempt, then jump to stage-2
 *
 * It holds the public key. The private half never existed on this device.
 */
#include "board.h"
#include "common/hal_boot.h"
#include "common/hal_boot_crypto.h"
#include "common/hal_bootmap.h"
#include "navhal.h"
#include "updater.h"
#include "verify_image.h"

#include <stdint.h>
#include <string.h>

/* Long enough to cover the slowest thing stage-1 does: hashing 384 KiB at 79
 * cycles/byte is 371 ms, and a recovery erase can take 2 s. The application's
 * own timeout is inherited across the reset and may be a tenth of that, which is
 * why this is set rather than assumed. */
#define STAGE1_WATCHDOG_MS 8000u

/* The CDC watch window. Enumeration measured 495 ms on this part; the margin is
 * for a slower host or an intervening hub. Bounded because a board with nothing
 * plugged into USB must still boot. */
#define STAGE1_CDC_WINDOW_MS 750u

static void say(const char *s) {
  hal_uart_print(BOARD_CONSOLE_UART, s);
}

/* Decimal, because "attempts=3" is the first thing anyone asks of a board that
 * went to recovery, and reading it over SWD instead is a bad trade for 40 bytes. */
static void say_u32(uint32_t v) {
  char b[11];
  int n = 0;
  if (v == 0u) {
    b[n++] = '0';
  }
  while (v != 0u) {
    b[n++] = (char)('0' + (v % 10u));
    v /= 10u;
  }
  char out[12];
  int m = 0;
  while (n > 0) {
    out[m++] = b[--n];
  }
  out[m] = 0;
  say(out);
}


/* Hand control to an image: its vector table first, then its stack and entry
 * point. VTOR has to move before the jump or the new image's interrupts would
 * vector into this one's table, which is write-protected and will not contain
 * its handlers. */
static void jump_to(uint32_t payload_base) {
  const uint32_t *vt = (const uint32_t *)payload_base;
  uint32_t sp = vt[0];
  uint32_t pc = vt[1];

  __asm__ volatile("cpsid i" ::: "memory");
  *(volatile uint32_t *)0xE000ED08UL = payload_base; /* SCB->VTOR */
  __asm__ volatile("dsb; isb" ::: "memory");
  __asm__ volatile("msr msp, %0\n bx %1" : : "r"(sp), "r"(pc));
  for (;;)
    ; /* not reached */
}

/* The two transports, as recovery wants them: one byte in, one byte out. Both
 * poll rather than interrupt -- stage-1 has no business installing handlers it
 * then has to tear down before the jump, and a loader waiting for bytes has
 * nothing better to do. */

static bool uart_get(uint8_t *out) {
  /* Bounded so a silent line does not wedge the session: a host that stops
   * mid-frame should cost a resync, not a reset. 2 s, because a human typing a
   * command by hand is a legitimate case. */
  uint32_t t0 = hal_timebase_get_millis();
  while (hal_timebase_get_millis() - t0 < 2000u) {
    (void)hal_watchdog_kick();
    if (hal_uart_available(BOARD_CONSOLE_UART)) {
      *out = (uint8_t)hal_uart_read_char(BOARD_CONSOLE_UART);
      return true;
    }
  }
  return false;
}

static void uart_put(uint8_t b) {
  (void)hal_uart_write_char(BOARD_CONSOLE_UART, (char)b);
}

#if NAVHAL_CONFIG_DRV_USB_CDC
static uint8_t cdc_rx_buf[64];
static volatile uint16_t cdc_rx_head;
static volatile uint16_t cdc_rx_tail;

static void cdc_rx(const uint8_t *data, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    uint16_t next = (uint16_t)((cdc_rx_head + 1u) % sizeof cdc_rx_buf);
    if (next == cdc_rx_tail)
      return; /* full: drop rather than overwrite unread bytes */
    cdc_rx_buf[cdc_rx_head] = data[i];
    cdc_rx_head = next;
  }
}

static bool cdc_get(uint8_t *out) {
  uint32_t t0 = hal_timebase_get_millis();
  while (hal_timebase_get_millis() - t0 < 2000u) {
    (void)hal_watchdog_kick();
    if (cdc_rx_tail != cdc_rx_head) {
      *out = cdc_rx_buf[cdc_rx_tail];
      cdc_rx_tail = (uint16_t)((cdc_rx_tail + 1u) % sizeof cdc_rx_buf);
      return true;
    }
  }
  return false;
}

static void cdc_put(uint8_t b) {
  (void)hal_usb_cdc_write(&b, 1u);
}
#endif

/* Recovery: say why, bring up whichever transports exist, then serve frames
 * until a RUN command resets the board. Never returns. */
static void recovery(const char *why) {
  say("\r\nstage1: recovery (");
  say(why);
  say(")\r\n");

#if NAVHAL_CONFIG_DRV_USB_CDC
  /* Enumerated, not connected: DTR needs an application to open the port, and a
   * board on a charger never asserts it. Bounded, because nothing may be
   * attached at all -- expiry is an ordinary outcome and UART still works. */
  (void)hal_usb_cdc_init();
  (void)hal_usb_cdc_set_rx_callback(cdc_rx);
  uint32_t waited = 0u;
  while (!hal_usb_cdc_enumerated() && waited < STAGE1_CDC_WINDOW_MS) {
    hal_delay_ms(10u);
    waited += 10u;
    (void)hal_watchdog_kick();
  }
  bool cdc = hal_usb_cdc_enumerated();
  say(cdc ? "stage1: cdc up\r\n" : "stage1: cdc absent\r\n");
#else
  const bool cdc = false;
#endif

  say("stage1: waiting for an image\r\n");

  /* Whichever speaks first wins the session. Trying UART first costs one timeout
   * when the host is on USB, which is 2 s of a recovery that is already manual. */
  static const recovery_transport_t uart_t = {.get = uart_get, .put = uart_put};
#if NAVHAL_CONFIG_DRV_USB_CDC
  static const recovery_transport_t cdc_t = {.get = cdc_get, .put = cdc_put};
#endif

  /* Stage-1 writes stage-2 and nothing else. No rollback floor here: the floor
   * guards the app, and the only thing that could roll stage-2 back is the stage
   * protected by WRP. */
  static const uint8_t stage2_sectors[] = {4u};
  static const updater_target_t target = {
      .base = HAL_BOOTMAP_STAGE2_BASE,
      .size = HAL_BOOTMAP_STAGE2_SIZE,
      .max_body = HAL_BOOTMAP_STAGE2_USABLE,
      .sectors = stage2_sectors,
      .sector_count = (uint8_t)(sizeof stage2_sectors / sizeof stage2_sectors[0]),
      .verify = boot_image_is_good,
      .floor_ok = NULL,
      .floor_set = NULL,
  };

  for (;;) {
    uint8_t b;
    (void)hal_watchdog_kick();

    if (hal_uart_available(BOARD_CONSOLE_UART)) {
      updater_serve(&uart_t, &target); /* never returns */
    }
#if NAVHAL_CONFIG_DRV_USB_CDC
    if (cdc && cdc_rx_tail != cdc_rx_head) {
      updater_serve(&cdc_t, &target); /* never returns */
    }
#else
    (void)cdc;
    (void)b;
#endif
  }
}

int main(void) {
  hal_clock_init_hz(HAL_CLOCK_SOURCE_HSE, 84000000u);
  hal_timebase_init(1000u);
  hal_uart_init(BOARD_CONSOLE_UART, &(hal_uart_config_t){.baudrate = 115200});

  say("\r\nstage1\r\n");

  /* Seed on a cold boot; a warm one keeps its request and attempt count. */
  (void)hal_boot_block_init();

  /* Adopt the watchdog rather than trusting it. The IWDG survives a system reset
   * and is cleared only by a power-on reset, so whatever timeout the application
   * chose is still running -- possibly 100 ms, against an erase that takes 2 s.
   * Starting it here sets the period AND makes the driver's own kick work: it
   * refuses while its internal flag is clear, which it is after a reset. */
  (void)hal_watchdog_start(STAGE1_WATCHDOG_MS);
  (void)hal_watchdog_kick();

  if (hal_boot_get_request() == HAL_BOOT_REQ_LOADER) {
    (void)hal_boot_clear_request(); /* consumed, so the next boot is normal */
    recovery("requested");
  }

  say("stage1: attempts=");
  say_u32(hal_boot_get_attempts());
  say("\r\n");

  if (hal_boot_get_attempts() >= HAL_BOOT_MAX_ATTEMPTS)
    recovery("crashloop");

  if (!boot_image_is_good(HAL_BOOTMAP_STAGE2_BASE, HAL_BOOTMAP_STAGE2_USABLE))
    recovery("stage-2 did not verify");

  /* Counted before the jump, not after: an image that faults immediately would
   * never come back to do it, and the count is what ends the loop. */
  (void)hal_boot_account_attempt();

  say("stage1: ok, jumping\r\n");
  jump_to(HAL_BOOTMAP_STAGE2_PAYLOAD);
  return 0;
}
