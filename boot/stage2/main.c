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
 * @brief Stage-2: verify the application, enforce the rollback floor, hand over
 *        or take an update.
 *
 * The replaceable stage. Stage-1 verified this image before jumping here, which
 * is why this one can be updated in the field while stage-1 cannot -- and why
 * the USB stack lives here rather than in the part protected by WRP.
 *
 * It inherits a running system: the clock is already set, the IWDG is already
 * going, and if stage-1 went through recovery the OTG core may already be
 * enumerated. Nothing here assumes otherwise, but nothing relies on it either.
 */
#include "board.h"
#include "common/hal_boot.h"
#include "common/hal_boot_crypto.h"
#include "common/hal_bootmap.h"
#include "navhal.h"
#include "updater.h"
#include "verify_image.h"

#include <stdint.h>

/* The KV key the rollback floor lives under. The store is in sectors 2-3, which
 * neither this stage nor stage-1 will erase -- the raw flash API refuses them. */
#define KV_KEY_ROLLBACK_FLOOR 0x21u

#define STAGE2_WATCHDOG_MS   8000u
#define STAGE2_CDC_WINDOW_MS 750u

static void say(const char *s) { hal_uart_print(BOARD_CONSOLE_UART, s); }

static void say_u32(uint32_t v) {
  char b[11];
  int n = 0;
  if (v == 0u)
    b[n++] = '0';
  while (v != 0u) {
    b[n++] = (char)('0' + (v % 10u));
    v /= 10u;
  }
  char out[12];
  int m = 0;
  while (n > 0)
    out[m++] = b[--n];
  out[m] = 0;
  say(out);
}

/* The floor: the lowest app version this board will still run. An absent key is
 * a floor of zero rather than an error -- a board that has never been updated has
 * nothing to roll back to, and refusing to boot because a record is missing would
 * turn a fresh unit into a brick. */
static uint32_t floor_get(void) {
  uint8_t buf[4] = {0};
  uint8_t size = sizeof buf;
  if (hal_flash_read(KV_KEY_ROLLBACK_FLOOR, buf, &size) != HAL_OK || size != 4u)
    return 0u;
  return (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) | ((uint32_t)buf[2] << 16) |
         ((uint32_t)buf[3] << 24);
}

static bool floor_ok(uint32_t version) { return version >= floor_get(); }

static bool floor_set(uint32_t version) {
  uint8_t buf[4] = {(uint8_t)(version & 0xFFu), (uint8_t)((version >> 8) & 0xFFu),
                    (uint8_t)((version >> 16) & 0xFFu),
                    (uint8_t)((version >> 24) & 0xFFu)};
  return hal_flash_save(KV_KEY_ROLLBACK_FLOOR, buf, sizeof buf) == HAL_OK;
}

static void jump_to(uint32_t payload_base) {
  const uint32_t *vt = (const uint32_t *)payload_base;
  uint32_t sp = vt[0];
  uint32_t pc = vt[1];
  __asm__ volatile("cpsid i" ::: "memory");
  *(volatile uint32_t *)0xE000ED08UL = payload_base; /* SCB->VTOR */
  __asm__ volatile("dsb; isb" ::: "memory");
  __asm__ volatile("msr msp, %0\n bx %1" : : "r"(sp), "r"(pc));
  for (;;)
    ;
}

/* --- transports, same shape as stage-1's --------------------------------- */

static bool uart_get(uint8_t *out) {
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
static uint8_t cdc_buf[64];
static volatile uint16_t cdc_head, cdc_tail;

static void cdc_rx(const uint8_t *data, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    uint16_t next = (uint16_t)((cdc_head + 1u) % sizeof cdc_buf);
    if (next == cdc_tail)
      return;
    cdc_buf[cdc_head] = data[i];
    cdc_head = next;
  }
}

static bool cdc_get(uint8_t *out) {
  uint32_t t0 = hal_timebase_get_millis();
  while (hal_timebase_get_millis() - t0 < 2000u) {
    (void)hal_watchdog_kick();
    if (cdc_tail != cdc_head) {
      *out = cdc_buf[cdc_tail];
      cdc_tail = (uint16_t)((cdc_tail + 1u) % sizeof cdc_buf);
      return true;
    }
  }
  return false;
}

static void cdc_put(uint8_t b) { (void)hal_usb_cdc_write(&b, 1u); }
#endif

/* The app's three sectors. A 384 KiB partition is not one erase unit, which is
 * why the server takes a list. */
static const uint8_t app_sectors[] = {5u, 6u, 7u};

static const updater_target_t app_target = {
    .base = HAL_BOOTMAP_APP_BASE,
    .size = HAL_BOOTMAP_APP_SIZE,
    .max_body = HAL_BOOTMAP_APP_USABLE,
    .sectors = app_sectors,
    .sector_count = (uint8_t)(sizeof app_sectors / sizeof app_sectors[0]),
    .verify = boot_image_is_good,
    .floor_ok = floor_ok,
    .floor_set = floor_set,
};

static void update_mode(const char *why) {
  say("\r\nstage2: update mode (");
  say(why);
  say(")\r\n");

#if NAVHAL_CONFIG_DRV_USB_CDC
  /* Stage-1 may already have enumerated on its way through recovery, in which
   * case init is a no-op and the wait ends immediately. Calling it regardless is
   * cheaper than asking. */
  (void)hal_usb_cdc_init();
  (void)hal_usb_cdc_set_rx_callback(cdc_rx);
  uint32_t waited = 0u;
  while (!hal_usb_cdc_enumerated() && waited < STAGE2_CDC_WINDOW_MS) {
    hal_delay_ms(10u);
    waited += 10u;
    (void)hal_watchdog_kick();
  }
  bool cdc = hal_usb_cdc_enumerated();
  say(cdc ? "stage2: cdc up\r\n" : "stage2: cdc absent\r\n");
#else
  const bool cdc = false;
#endif

  say("stage2: waiting for an app image\r\n");

  static const recovery_transport_t uart_t = {.get = uart_get, .put = uart_put};
#if NAVHAL_CONFIG_DRV_USB_CDC
  static const recovery_transport_t cdc_t = {.get = cdc_get, .put = cdc_put};
#endif

  for (;;) {
    (void)hal_watchdog_kick();
    if (hal_uart_available(BOARD_CONSOLE_UART))
      updater_serve(&uart_t, &app_target);
#if NAVHAL_CONFIG_DRV_USB_CDC
    if (cdc && cdc_tail != cdc_head)
      updater_serve(&cdc_t, &app_target);
#else
    (void)cdc;
#endif
  }
}

int main(void) {
  hal_clock_init_hz(HAL_CLOCK_SOURCE_HSE, 84000000u);
  hal_timebase_init(1000u);
  hal_uart_init(BOARD_CONSOLE_UART, &(hal_uart_config_t){.baudrate = 115200});
  (void)hal_watchdog_start(STAGE2_WATCHDOG_MS);
  (void)hal_watchdog_kick();

  say("\r\nstage2\r\n");

  (void)hal_boot_block_init();

  if (hal_boot_get_request() == HAL_BOOT_REQ_LOADER) {
    (void)hal_boot_clear_request();
    update_mode("requested");
  }

  if (!boot_image_is_good(HAL_BOOTMAP_APP_BASE, HAL_BOOTMAP_APP_USABLE))
    update_mode("app did not verify");

  const hal_boot_image_header_t *h =
      (const hal_boot_image_header_t *)HAL_BOOTMAP_APP_BASE;
  say("stage2: app version=");
  say_u32(h->version);
  say(" floor=");
  say_u32(floor_get());
  say("\r\n");

  /* Signed is not the same as current. An old image with a valid signature is
   * exactly what a downgrade attack looks like, and the signature cannot tell
   * them apart -- only the floor can. */
  if (!floor_ok(h->version))
    update_mode("app is older than the rollback floor");

  /* The floor advances to what is about to run, not to whatever was offered: an
   * image is only allowed to raise the bar once it is the one being booted. */
  if (h->version > floor_get()) {
    if (floor_set(h->version)) {
      say("stage2: floor raised\r\n");
    } else {
      /* Not fatal. The image verified and is current; a store that refused the
       * write leaves the floor where it was, which is conservative. */
      say("stage2: floor write refused\r\n");
    }
  }

  say("stage2: ok, jumping to the app\r\n");
  jump_to(HAL_BOOTMAP_APP_PAYLOAD);
  return 0;
}
