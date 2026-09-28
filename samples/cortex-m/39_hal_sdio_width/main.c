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
 * @brief Report why a card reads in 1-bit and not in 4-bit, in one run.
 *
 * @details
 * A diagnostic, not a demonstration. 4-bit transfers fail on some cards while
 * 1-bit works, and the interesting question is whether the *card* ever agreed to
 * the width the host switched to. This asks both widths back to back and prints
 * what it observed rather than what it assumed:
 *
 *   - the card's own R1 status after ACMD6, in hex — the number that settles it
 *   - the WIDBUS field read back out of CLKCR, so host state is observed
 *   - a 512-byte read in 4-bit, then the same read in 1-bit as the control
 *
 * Both widths in one image on purpose. Reflashing between them would let a card
 * left in the data state by a failed transfer poison the next attempt, which is
 * how "it fails at 400 kHz too" and "it fails with flow control off" can both be
 * true while neither setting is the cause.
 *
 * ### Reading the result
 *
 * | 4-bit | 1-bit | means |
 * |---|---|---|
 * | ok | ok | 4-bit works; whatever was wrong is fixed |
 * | fail | ok | the card refused the width, or the data lines do not carry — the R1 above says which |
 * | fail | fail | not a width problem at all; look at the card or the slot |
 *
 * ### Console
 *
 * Prints through ::hal_console_write, so it follows the build's CONSOLE_ROUTE.
 * On a board whose console UART reaches no host, build it with
 * `CONFIG_CONSOLE_ROUTE_CDC=y` and read it on the USB device port; the loop waits
 * for enumeration first, because anything written before then is dropped.
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

/* 48 MHz / (118 + 2) = 400 kHz, the rate a card must be identified at. */
#define SD_IDENT_DIV 118

static uint8_t block[512];

static void say(const char *s) { hal_console_write(s); }

static void say_hex(const char *label, uint32_t v) {
  say(label);
  hal_console_write_hex32(v);
  say("\r\n");
}

/* WIDBUS is bits 12:11 of CLKCR: 0 = 1-bit, 1 = 4-bit, 2 = 8-bit. Read back
 * rather than inferred -- the point of the exercise is to compare what the host
 * believes against what the card agreed to. */
static uint32_t host_width_bits(void) { return (SDIO->CLKCR >> 11) & 0x3u; }

static void report_width(void) {
  uint32_t w = host_width_bits();
  say("  host WIDBUS       : ");
  hal_console_write_uint(w);
  say(w == 0u ? " (1-bit)\r\n" : (w == 1u ? " (4-bit)\r\n" : " (8-bit)\r\n"));
}

/* One block read, reported by outcome rather than by contents. Sector 0 is the
 * MBR on any formatted card and is always readable, so a failure here is the
 * bus, not the filesystem. */
static void try_read(const char *what) {
  say(what);
  hal_sdio_error_t err = hal_sdio_read_block(0u, block);
  if (err == HAL_SDIO_OK) {
    say("  read sector 0     : OK, first bytes ");
    hal_console_write_hex32(((uint32_t)block[0] << 24) | ((uint32_t)block[1] << 16) |
                            ((uint32_t)block[2] << 8) | block[3]);
    say("\r\n");
  } else {
    say("  read sector 0     : FAILED, hal_sdio_error_t = ");
    hal_console_write_uint((uint32_t)err);
    say("\r\n");
  }
}

static void attempt(uint8_t bus_width, const char *title) {
  hal_sdio_config_t cfg = {.clock_div = SD_IDENT_DIV, .bus_width = bus_width};

  say(title);
  if (hal_sdio_init(&cfg) != HAL_SDIO_OK) {
    say("  hal_sdio_init     : FAILED\r\n");
    return;
  }
  hal_sdio_error_t err = hal_sdio_card_init();
  if (err != HAL_SDIO_OK) {
    say("  hal_sdio_card_init: FAILED, hal_sdio_error_t = ");
    hal_console_write_uint((uint32_t)err);
    say("\r\n");
    return;
  }
  say("  hal_sdio_card_init: OK\r\n");
  say_hex("  last R1 (RESP1)   : ", SDIO->RESP1);
  report_width();
  say("  sectors           : ");
  hal_console_write_uint(hal_sdio_get_sector_count());
  say("\r\n");
  try_read("  -- reading --\r\n");
}

int main(void) {
  clk_cfg.pll = pll_cfg;
  hal_clock_init(&clk_cfg);
  hal_timebase_init(1000);

  hal_console_init(115200);

  /* On the CDC route nothing written before the host attaches is delivered, and
   * the whole value of this sample is its output. On the UART route this returns
   * immediately. */
  while (!hal_console_connected()) {
    hal_delay_ms(50);
  }
  hal_delay_ms(200); /* let the host's port settle before the first line */

  say("\r\n=== NavHAL SDIO bus-width diagnostic ===\r\n");
  say("console route      : ");
  say(hal_console_get_route() == HAL_CONSOLE_ROUTE_CDC ? "USB CDC\r\n" : "UART\r\n");

  attempt(1u, "\r\n[1] requesting 4-bit\r\n");
  attempt(0u, "\r\n[2] requesting 1-bit (control)\r\n");

  say("\r\n=== done ===\r\n");
  while (1) {
    hal_delay_ms(1000);
  }
}
