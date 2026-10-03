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
#include "stage1_pubkey.h"

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

/* Verify the image at @p base against @p max_body. The order matters: magic,
 * then the length bound, then the hash, then the signature. Hashing before the
 * length is checked is how a verifier gets walked off the end of flash into
 * bytes an attacker chose. */
static bool image_is_good(uint32_t base, uint32_t max_body) {
  const hal_boot_image_header_t *h = (const hal_boot_image_header_t *)base;

  if (h->magic != (uint32_t)HAL_BOOTMAP_IMAGE_MAGIC)
    return false;
  if (h->length == 0u || h->length > max_body)
    return false;

  /* The digest covers the header's first 12 bytes and then the body, which are
   * not contiguous -- the padding sits between them -- so the hash is fed in two
   * parts rather than over one range. */
  uint8_t digest[HAL_BOOT_DIGEST_SIZE];
  if (hal_boot_hash_split(digest, (const uint8_t *)base,
                          HAL_BOOTMAP_SIGNED_PREFIX,
                          (const uint8_t *)(base + HAL_BOOTMAP_HEADER_SIZE),
                          h->length) != HAL_OK)
    return false;

  if (memcmp(digest, h->digest, sizeof digest) != 0)
    return false;

  return hal_boot_ed25519_verify(h->sig, stage1_pubkey, digest,
                                 sizeof digest) == HAL_OK;
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

/* Recovery. Slice 5b adds the receive protocol; for now it reports which
 * transport is available and waits, which is still better than jumping into an
 * image that failed to verify. */
static void recovery(const char *why) {
  say("\r\nstage1: recovery (");
  say(why);
  say(")\r\n");

#if NAVHAL_CONFIG_DRV_USB_CDC
  /* Enumerated, not connected: DTR needs an application to open the port, and a
   * board on a charger never asserts it. Bounded, because nothing may be
   * attached at all. */
  (void)hal_usb_cdc_init();
  uint32_t waited = 0u;
  while (!hal_usb_cdc_enumerated() && waited < STAGE1_CDC_WINDOW_MS) {
    hal_delay_ms(10u);
    waited += 10u;
    (void)hal_watchdog_kick();
  }
  say(hal_usb_cdc_enumerated() ? "stage1: cdc up\r\n" : "stage1: cdc absent\r\n");
#endif

  for (;;) {
    (void)hal_watchdog_kick();
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

  if (!image_is_good(HAL_BOOTMAP_STAGE2_BASE, HAL_BOOTMAP_STAGE2_USABLE))
    recovery("stage-2 did not verify");

  /* Counted before the jump, not after: an image that faults immediately would
   * never come back to do it, and the count is what ends the loop. */
  (void)hal_boot_account_attempt();

  say("stage1: ok, jumping\r\n");
  jump_to(HAL_BOOTMAP_STAGE2_PAYLOAD);
  return 0;
}
