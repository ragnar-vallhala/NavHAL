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
 * ### What it found
 *
 * On NAVIXSM-F401RE v0.1.0: DAT3 (PC11) never went low while the card
 * transmitted, so the DPSM's start-bit detect -- which needs DAT0..DAT3 low
 * together -- never fired. 4-bit moved no data at any clock while 1-bit moved all
 * of it. An open joint on the PC11 / R14 / Card1.2 path; resoldering it fixed
 * 4-bit at every rate from 400 kHz to 12 MHz.
 *
 * Five software hypotheses died before that: the card refusing ACMD6, an
 * unbounded wait, hardware flow control, the sampling edge, and clock speed. The
 * signature table below is what finally separated them.
 *
 * ### Reading the result
 *
 * Build one width per boot with -DDIAG_BUS_WIDTH=0 or 1: hal_sdio_card_init()
 * early-returns on a static flag, so only the first attempt in a run ever talks to
 * the card, and a second attempt at another width reads a card still set to the
 * first one.
 *
 * | STA after a failed read | means |
 * |---|---|
 * | `STBITERR`, DCOUNT unchanged | host wider than the card is transmitting: a line never goes low |
 * | `DCRCFAIL`, DCOUNT zero | host narrower than the card: all the data arrived, sampled on too few lines |
 * | `DTIMEOUT` | the card never answered at all |
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

/* Every line also lands in RAM, at a fixed marker, so the report survives losing
 * the console. The 4-bit read takes USB CDC down with it on this board -- the
 * capture ends mid-read with the device disconnected -- and a diagnostic whose
 * output disappears exactly when the interesting thing happens is no use. Read it
 * out with the debugger instead:
 *
 *   openocd ... -c "init; halt; mdw 0x2000xxxx 256; exit"
 *
 * The marker is how the buffer is found without a symbol table. */
#define DIAG_LOG_BYTES 4096u
volatile char diag_log[DIAG_LOG_BYTES] __attribute__((used)) = "NAVHALDIAG";
volatile uint32_t diag_len __attribute__((used)) = 10u; /* past the marker */

static void log_ram(const char *s) {
  while (*s != '\0' && diag_len < (DIAG_LOG_BYTES - 1u)) {
    diag_log[diag_len++] = *s++;
  }
  diag_log[diag_len] = '\0';
}

/* RAM first, unconditionally, then the console if there is one. The RAM copy must
 * not depend on a host being attached: gating the whole report on enumeration is
 * how an earlier run of this sample produced nothing at all to read back. */
static void say(const char *s) {
  log_ram(s);
  if (hal_console_connected()) {
    hal_console_write(s);
  }
}

/* The number helpers write straight to the console, so render into RAM too. */
static void say_hex(const char *label, uint32_t v) {
  static const char digits[] = "0123456789ABCDEF";
  char buf[11];
  buf[0] = '0'; buf[1] = 'x';
  for (unsigned i = 0u; i < 8u; i++) {
    buf[2u + i] = digits[(v >> (28u - 4u * i)) & 0xFu];
  }
  buf[10] = '\0';
  say(label);
  say(buf);
  say("\r\n");
}

static void say_uint(uint32_t v) {
  char buf[12];
  unsigned i = 11u;
  buf[i] = '\0';
  if (v == 0u) { buf[--i] = '0'; }
  while (v != 0u && i > 0u) { buf[--i] = (char)('0' + (v % 10u)); v /= 10u; }
  say(buf + i);
}

/* WIDBUS is bits 12:11 of CLKCR: 0 = 1-bit, 1 = 4-bit, 2 = 8-bit. Read back
 * rather than inferred -- the point of the exercise is to compare what the host
 * believes against what the card agreed to. */
static uint32_t host_width_bits(void) { return (SDIO->CLKCR >> 11) & 0x3u; }

static void report_width(void) {
  uint32_t w = host_width_bits();
  say("  host WIDBUS       : ");
  say_uint(w);
  say(w == 0u ? " (1-bit)\r\n" : (w == 1u ? " (4-bit)\r\n" : " (8-bit)\r\n"));
}

/* The DPSM state, captured either side of the read. A failure here is the data
 * path, and the data path is these five registers: what length and block size the
 * host asked for, what timeout it allowed, what clock and bus width it is running,
 * and which status bits the hardware raised. Printed raw -- the point is to compare
 * them against the reference manual, not against an interpretation. */
static void dump_dpsm(const char *when) {
  say(when);
  say_hex("    SDIO_CLKCR      : ", SDIO->CLKCR);
  say_hex("    SDIO_DCTRL      : ", SDIO->DCTRL);
  say_hex("    SDIO_DLEN       : ", SDIO->DLEN);
  say_hex("    SDIO_DTIMER     : ", SDIO->DTIMER);
  say_hex("    SDIO_STA        : ", SDIO->STA);
  say_hex("    SDIO_DCOUNT     : ", SDIO->DCOUNT);
  say_hex("    SDIO_RESPCMD    : ", SDIO->RESPCMD);
}

/* Which data line never moves while the card is transmitting.
 *
 * The card sends a block on all four lines once ACMD6 has widened it. Take the
 * lines away from the DPSM, issue CMD17 by hand, and watch them as plain inputs:
 * every line the card can actually reach goes low at least once, for the start bit
 * if nothing else. One that stays high for the whole burst is not connected to the
 * card -- the pull-up holds it, the card's driver is on the far side of a break,
 * and the DPSM's start-bit detect never sees all four low. That is STBITERR, and
 * it is why a 4-bit read moves no data while a 1-bit read moves all of it.
 *
 * Deliberately no DPSM setup: the card transmits in response to CMD17 whether or
 * not the host is configured to receive, which is what makes this observable.
 */
static void find_dead_line(void) {
  static const hal_gpio_pin pins[4] = {GPIO_PC08, GPIO_PC09, GPIO_PC10, GPIO_PC11};
  static const char *names[4] = {"    DAT0 (PC8)     : ", "    DAT1 (PC9)     : ",
                                 "    DAT2 (PC10)    : ", "    DAT3 (PC11)    : "};
  bool went_low[4] = {false, false, false, false};

  say("  -- which line never goes low while the card transmits? --\r\n");
  for (unsigned i = 0u; i < 4u; i++) {
    hal_gpio_set_mode(pins[i], HAL_GPIO_MODE_INPUT, HAL_GPIO_PULL_UP);
  }

  (void)hal_sdio_send_command(SD_CMD_READ_SINGLE_BLOCK, 0u, 1);

  /* Sample hard for long enough to cover a block at the current clock. */
  for (uint32_t n = 0u; n < 400000u; n++) {
    for (unsigned i = 0u; i < 4u; i++) {
      if (hal_gpio_read(pins[i]) == HAL_GPIO_LOW) {
        went_low[i] = true;
      }
    }
  }

  for (unsigned i = 0u; i < 4u; i++) {
    say(names[i]);
    say(went_low[i] ? "toggled low - reaches the card\r\n"
                    : "STUCK HIGH  - no connection to the card\r\n");
  }

  for (unsigned i = 0u; i < 4u; i++) {
    hal_gpio_set_alternate_function(pins[i], HAL_GPIO_AF12);
    hal_gpio_set_output_speed(pins[i], HAL_GPIO_SPEED_VERY_HIGH);
    hal_gpio_set_output_type(pins[i], HAL_GPIO_OTYPE_PUSH_PULL);
    hal_gpio_set_mode(pins[i], HAL_GPIO_MODE_AF, HAL_GPIO_PULL_UP);
  }
  /* The card was left mid-stream by a CMD17 nobody received. */
  (void)hal_sdio_send_command(SD_CMD_STOP_TRANSMISSION, 0u, 1);
}

/* One block read, reported by outcome rather than by contents. Sector 0 is the
 * MBR on any formatted card and is always readable, so a failure here is the
 * bus, not the filesystem. */
static void try_read(const char *what) {
  say(what);
  dump_dpsm("  before read:\r\n");
  hal_sdio_error_t err = hal_sdio_read_block(0u, block);
  dump_dpsm("  after read:\r\n");
  if (err == HAL_SDIO_OK) {
    say("  read sector 0     : OK, first bytes ");
    say_hex("", ((uint32_t)block[0] << 24) | ((uint32_t)block[1] << 16) |
                 ((uint32_t)block[2] << 8) | block[3]);
  } else {
    say("  read sector 0     : FAILED, hal_sdio_error_t = ");
    say_uint((uint32_t)err);
    say("\r\n");
  }
}

/* Force the bus to a known width and clock, and report what stuck.
 *
 * hal_sdio_card_init() has a static `initialized` flag and returns immediately on
 * a second call, so the second attempt never re-identifies the card and never
 * reaches the CLKCR write that sets the run clock. Left alone, attempt 1 read at
 * 12 MHz in 4-bit and attempt 2 at 400 kHz in 1-bit -- two variables changed at
 * once, which is not a comparison. Set both explicitly so only the width differs. */
static void force_bus(uint32_t widbus, uint32_t clkdiv, uint32_t negedge) {
  SDIO->CLKCR = (SDIO->CLKCR & ~(SDIO_CLKCR_CLKDIV | SDIO_CLKCR_WIDBUS_Msk |
                                 SDIO_CLKCR_HWFC_EN | SDIO_CLKCR_PWRSAV |
                                 SDIO_CLKCR_NEGEDGE)) |
                (clkdiv & 0xFFu) | (widbus << 11) | SDIO_CLKCR_CLKEN |
                (negedge ? SDIO_CLKCR_NEGEDGE : 0u);
}

static void attempt(hal_sdio_bus_width_t bus_width, uint32_t negedge,
                    const char *title) {
  hal_sdio_config_t cfg = {.clock_div = SD_IDENT_DIV, .bus_width = bus_width};

  say(title);
  if (hal_sdio_init(&cfg) != HAL_SDIO_OK) {
    say("  hal_sdio_init     : FAILED\r\n");
    return;
  }
  hal_sdio_error_t err = hal_sdio_card_init();
  if (err != HAL_SDIO_OK) {
    say("  hal_sdio_card_init: FAILED, hal_sdio_error_t = ");
    say_uint((uint32_t)err);
    say("\r\n");
    return;
  }
  say("  hal_sdio_card_init: OK\r\n");
  say_hex("  last R1 (RESP1)   : ", SDIO->RESP1);
  report_width();
  find_dead_line();
  say("  sectors           : ");
  say_uint(hal_sdio_get_sector_count());
  say("\r\n");

  /* Sweep the clock at the width the card was identified in. The card keeps the
   * width agreed by ACMD6 until it is re-identified, so the divider is the only
   * thing changing between these reads -- which makes this the one comparison the
   * early-returning card_init still allows in a single boot.
   *
   * 1-bit reads succeed at CLKDIV=2 (12 MHz), so if 4-bit succeeds at a low clock
   * and fails at a high one, the extra lines are an integrity problem at speed
   * rather than a protocol one. If it fails at every rate, speed is not involved
   * at all. */
  {
    static const uint32_t divs[4] = {118u, 40u, 10u, 2u};
    static const char *rates[4] = {"400 kHz", "1.17 MHz", "4 MHz", "12 MHz"};
    for (unsigned i = 0u; i < 4u; i++) {
      force_bus((bus_width == HAL_SDIO_BUS_WIDTH_4BIT) ? 1u : 0u, divs[i], negedge);
      say("  -- CLKDIV ");
      say_uint(divs[i]);
      say(" (");
      say(rates[i]);
      say(") --\r\n");
      hal_sdio_error_t e = hal_sdio_read_block(0u, block);
      say("    read sector 0   : ");
      if (e == HAL_SDIO_OK) {
        say("OK\r\n");
      } else {
        say("FAILED err=");
        say_uint((uint32_t)e);
        say_hex("  STA=", SDIO->STA);
      }
    }
  }

  /* THE discriminator. The card keeps whatever width ACMD6 agreed until it is
   * re-identified, so with the host forced back to one line:
   *
   *   read succeeds -> the card is still in 1-bit. ACMD6 reported success and did
   *                    not take effect, and the host has been listening on four
   *                    lines to a card talking on one -- which is precisely
   *                    STBITERR at every clock rate, with every line connected.
   *   read fails    -> the card really is in 4-bit, and the fault is in receiving
   *                    DAT1..DAT3 despite them being driven.
   */
  /* The control for the discriminator below: here the card is known to be in
   * 1-bit, because that is what it was identified in. Forcing the host to four
   * lines creates a deliberate mismatch, so whatever the hardware reports for
   * THIS is the signature of "host wider than card" -- and can be compared
   * against what the 4-bit boot reports. */
  if (bus_width == HAL_SDIO_BUS_WIDTH_1BIT) {
    force_bus(1u, 2u, negedge);
    say("  -- control: card is 1-bit, host forced to 4-bit --\r\n");
    hal_sdio_error_t e = hal_sdio_read_block(0u, block);
    say("    read sector 0   : ");
    if (e == HAL_SDIO_OK) {
      say("OK (unexpected)\r\n");
    } else {
      say("FAILED err=");
      say_uint((uint32_t)e);
      say_hex("  STA=", SDIO->STA);
    }
    force_bus(0u, 2u, negedge);
  }

  if (bus_width == HAL_SDIO_BUS_WIDTH_4BIT) {
    force_bus(0u, 2u, negedge);
    say("  -- host forced to 1-bit, card left as ACMD6 set it --\r\n");
    hal_sdio_error_t e = hal_sdio_read_block(0u, block);
    say("    read sector 0   : ");
    if (e == HAL_SDIO_OK) {
      say("OK -> the card is in 1-BIT; ACMD6 never took effect\r\n");
    } else {
      say("FAILED err=");
      say_uint((uint32_t)e);
      say_hex("  STA=", SDIO->STA);
      say("    -> the card is in 4-bit; the fault is receiving DAT1..DAT3\r\n");
    }
  }

  /* Same clock everywhere; width and sampling edge are the only variables.
   *
   * NEGEDGE selects which SDIO_CK edge the host drives and samples on, and the
   * driver has never set it. A wrong edge corrupts data at *every* clock rate
   * rather than only fast ones, which is the one story that fits both observed
   * failures: 1-bit transferring all 512 bytes with a bad CRC at 400 kHz and at
   * 12 MHz alike, and 4-bit never seeing a start bit simultaneously on four
   * lines. Speed, flow control and the card itself have all been eliminated. */
  force_bus((bus_width == HAL_SDIO_BUS_WIDTH_4BIT) ? 1u : 0u, 2u, negedge);
  say("  forced bus        : ");
  say_uint((bus_width == HAL_SDIO_BUS_WIDTH_4BIT) ? 4u : 1u);
  say(negedge ? "-bit @ CLKDIV 2, NEGEDGE=1\r\n" : "-bit @ CLKDIV 2, NEGEDGE=0\r\n");
  report_width();
  /* Capacity class is not observable from outside the driver -- card_is_sdhc and
   * the cached CSD are file-static in sdio.c -- so it is not reported here rather
   * than guessed at. The sector count above is derived from the CSD, so a sane
   * value means the CSD was read correctly whatever the class. */
  try_read("  -- reading --\r\n");
}

int main(void) {
  clk_cfg.pll = pll_cfg;
  hal_clock_init(&clk_cfg);
  hal_timebase_init(1000);

  hal_console_init(115200);

  /* Give a host a few seconds to attach so the live console is usually there,
   * but never wait on it: the report goes to RAM regardless, and a diagnostic
   * that refuses to run without an audience is useless on the failure this one
   * exists to inspect. */
  for (uint32_t i = 0u; i < 60u && !hal_console_connected(); i++) {
    hal_delay_ms(50);
  }
  hal_delay_ms(200); /* let the host's port settle before the first line */

  say("\r\n=== NavHAL SDIO bus-width diagnostic ===\r\n");

  /* Card-detect polarity, measured rather than assumed. These switches are
   * usually to ground with a pull-up, so LOW means inserted -- but that is the
   * convention, not this socket's datasheet. Read it with both internal pulls: a
   * line that follows the pull is floating (not connected), one that reads the
   * same either way is being held, and which level it is held at is the answer. */
#if defined(BOARD_SD_CD)
  hal_gpio_set_mode(BOARD_SD_CD, HAL_GPIO_MODE_INPUT, HAL_GPIO_PULL_UP);
  hal_delay_ms(2);
  bool cd_pu = (hal_gpio_read(BOARD_SD_CD) == HAL_GPIO_HIGH);
  hal_gpio_set_mode(BOARD_SD_CD, HAL_GPIO_MODE_INPUT, HAL_GPIO_PULL_DOWN);
  hal_delay_ms(2);
  bool cd_pd = (hal_gpio_read(BOARD_SD_CD) == HAL_GPIO_HIGH);
  say("card detect (PC5) : pull-up reads ");
  say(cd_pu ? "HIGH" : "LOW");
  say(", pull-down reads ");
  say(cd_pd ? "HIGH" : "LOW");
  say(cd_pu == cd_pd ? "  -> held, so connected\r\n"
                     : "  -> follows the pull, so floating\r\n");
#else
  say("card detect        : board defines no BOARD_SD_CD\r\n");
#endif
  say("console route      : ");
  say(hal_console_get_route() == HAL_CONSOLE_ROUTE_CDC ? "USB CDC\r\n" : "UART\r\n");

  /* ONE width per boot, chosen at build time.
   *
   * Four attempts in a single run looked thorough and was worthless:
   * hal_sdio_card_init() early-returns on its static `initialized` flag, so only
   * the first attempt ever talks to the card. Attempt 1 asked for 4-bit, the card
   * agreed via ACMD6 and *stayed* in 4-bit -- so every later attempt that forced
   * the host to 1-bit was reading a 4-bit card one line at a time. That produces
   * a full-length transfer with a wrong CRC, which is exactly what was observed
   * and attributed to the bus. The card's width can only be changed by
   * re-identifying it, and that needs a reset.
   *
   * Build with -DDIAG_BUS_WIDTH=0 or 1 and run each separately. */
#ifndef DIAG_BUS_WIDTH
#define DIAG_BUS_WIDTH 1
#endif
  attempt(DIAG_BUS_WIDTH ? HAL_SDIO_BUS_WIDTH_4BIT : HAL_SDIO_BUS_WIDTH_1BIT, 0u,
          (DIAG_BUS_WIDTH ? "\r\n[single] 4-bit requested, NEGEDGE=0\r\n"
                          : "\r\n[single] 1-bit requested, NEGEDGE=0\r\n"));

  say("\r\n=== done ===\r\n");
  while (1) {
    hal_delay_ms(1000);
  }
}
