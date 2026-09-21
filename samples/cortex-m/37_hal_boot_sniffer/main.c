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
 * @brief Boot watcher on UART and CDC: type the magic sequence, board reboots.
 *
 * @details
 * The half of the bootloader design that lives in the application. Both
 * consoles are watched for the magic sequence; on a match the board records a
 * request in the boot block and resets, and the next boot reports the request
 * it found. That report is the whole point — it is the evidence that four
 * words of RAM survived a reset, which is what the bootloader will depend on.
 *
 * There is no bootloader yet, so this sample stands in for stage-1: it reads
 * the request, prints it, and clears it. When stage-1 exists it does exactly
 * that and stays in update mode instead of starting the application.
 *
 * ### Why this is a sample and not a test
 *
 * The committed test suites deliberately never call ::hal_boot_request,
 * because it resets the board and would end the run. Here the reset is the
 * result rather than a casualty — the same reason
 * samples/portable/36_hal_watchdog exists.
 *
 * ### Wiring
 *
 * UART is fed by circular DMA plus the IDLE interrupt, which is the only
 * receive path here that keeps running when the main loop does not. That is
 * the case the watcher exists for, so it is the arrangement the sample shows:
 * the DMA fills the ring with no CPU involvement, and the IDLE interrupt wakes
 * a walker on each frame gap.
 *
 * CDC is fed by its RX callback, which must forward — registering a callback
 * there takes the stream over rather than tapping it, so a sample that echoed
 * and also registered the watcher naively would echo nothing.
 *
 * ### Trying it
 *
 * @code
 * screen /dev/ttyACM0 9600            # the Nucleo's ST-Link virtual COM port
 * printf '\xB0\x07\xC0\xDE\xA5\x3C\x69\x96' > /dev/ttyACM0
 * @endcode
 *
 * The LED goes out, the board resets, and the banner comes back reporting that
 * the previous boot asked for the loader. Holding the user button refuses the
 * request instead: that is ::hal_boot_entry_disable, which on a vehicle is
 * what the airframe arming does, since rebooting in flight is a fall.
 *
 * USB full speed needs exactly 48 MHz, so the PLL is set up first:
 * 8 MHz HSE / M=8 -> 1 MHz, * N=336 -> 336 MHz VCO, / P=4 -> 84 MHz SYSCLK,
 * / Q=7 -> 48 MHz USB clock.
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

/* The ring the UART's DMA fills. Nothing reads it but the walker below, and
 * nothing writes it but the DMA controller. */
#define RX_RING_LEN 128u
static uint8_t rx_ring[RX_RING_LEN];
static uint16_t rx_tail;

static void print(const char *s) { hal_uart_write_string(BOARD_CONSOLE_UART, s); }

static void print_u32(uint32_t v) { hal_uart_write_uint(BOARD_CONSOLE_UART, v); }

/* -------------------------------------------------------------------------- *
 * Feeds
 * -------------------------------------------------------------------------- */

/**
 * IDLE fires at the end of each burst of bytes. Everything the DMA has landed
 * since the last visit is between rx_tail and the controller's write index, so
 * one pass over that span feeds the matcher without the main loop being
 * involved at all.
 */
static void on_uart_idle(void) { /* ISR context */
  uint16_t head;

  if (hal_uart_dma_rx_index(BOARD_CONSOLE_UART, &head) != HAL_OK) {
    return;
  }
  while (rx_tail != head) {
    hal_boot_match_byte(rx_ring[rx_tail]);
    rx_tail = (uint16_t)((rx_tail + 1u) % RX_RING_LEN);
  }
}

#if NAVHAL_CONFIG_DRV_USB_CDC
/**
 * hal_usb_cdc_set_rx_callback delivers bytes here *instead of* queueing them
 * for hal_usb_cdc_read, so whatever this does not pass on is lost. Feed the
 * watcher, then echo, in that order: an echo that blocked would otherwise
 * delay the match.
 */
static void on_cdc_rx(const uint8_t *data, uint16_t len) { /* ISR context */
  hal_boot_feed(data, len);
  (void)hal_usb_cdc_write(data, len);
}
#endif

/* -------------------------------------------------------------------------- *
 * Policy
 * -------------------------------------------------------------------------- */

/**
 * Runs immediately before the reset, in the interrupt the matching byte
 * arrived on. Putting the LED out stands in for cutting throttle: short,
 * interrupt-safe, and done before the board goes away.
 *
 * It deliberately does not print. A blocking console write from an ISR is the
 * wrong shape for the thing this hook is for, and on a vehicle the microsecond
 * spent on it is a microsecond the motors are still turning.
 */
static void on_boot_request(void) { hal_gpio_write(LED_BUILTIN, LED_OFF); }

/** The user button stands in for "the airframe is armed". Active low. */
static bool entry_should_be_refused(void) {
  return hal_gpio_read(USER_BUTTON) == HAL_GPIO_LOW;
}

/* -------------------------------------------------------------------------- *
 * Startup
 * -------------------------------------------------------------------------- */

static void report_previous_boot(void) {
  uint32_t request = hal_boot_get_request();

  print("\r\n--- NavHAL boot watcher ---\r\n");
  print("reset cause: ");
  print_u32(hal_reset_get_cause());
  print("\r\nprevious boot requested: ");

  if (request == HAL_BOOT_REQ_LOADER) {
    /* Stage-1 would stay in update mode here instead of starting the app. */
    print("LOADER -- the boot block survived the reset\r\n");
    (void)hal_boot_clear_request();
  } else {
    print("nothing (normal boot)\r\n");
  }

  print("attempts: ");
  print_u32(hal_boot_get_attempts());
  print("\r\nsend B0 07 C0 DE A5 3C 69 96 to reboot into the loader\r\n");
  print("hold the user button to refuse the request instead\r\n\r\n");
}

int main(void) {
  clk_cfg.pll = pll_cfg;
  hal_clock_init(&clk_cfg);
  hal_timebase_init(1000);

  hal_gpio_set_mode(LED_BUILTIN, HAL_GPIO_MODE_OUTPUT, HAL_GPIO_PULL_NONE);
  hal_gpio_write(LED_BUILTIN, LED_ON);
  hal_gpio_set_mode(USER_BUTTON, HAL_GPIO_MODE_INPUT, HAL_GPIO_PULL_UP);

  hal_reset_init(); /* latch and clear the cause before anything else resets */
  hal_boot_block_init();
  hal_boot_set_prepare(on_boot_request);

  hal_uart_init(BOARD_CONSOLE_UART, &(hal_uart_config_t){.baudrate = 9600});
  report_previous_boot();

  /* The watcher's real feed: DMA fills the ring, IDLE walks it. Neither needs
   * this loop to be running, which is the entire point. */
  if (hal_uart_init_dma_rx(BOARD_CONSOLE_UART, rx_ring, RX_RING_LEN) != HAL_OK) {
    print("RX DMA unavailable -- the watcher would depend on this loop\r\n");
  }
  hal_uart_attach_idle_callback(BOARD_CONSOLE_UART, on_uart_idle);

#if NAVHAL_CONFIG_DRV_USB_CDC
  if (hal_usb_cdc_init() == HAL_OK) {
    hal_usb_cdc_set_rx_callback(on_cdc_rx);
    print("CDC up: the same sequence works on /dev/ttyACM* too\r\n");
  }
#endif

  /* Heartbeat, and the entry gate tracking the button. The watcher itself runs
   * entirely in interrupts -- this loop could stop and the sequence would
   * still reboot the board. */
  bool refused = false;
  while (1) {
    bool refuse_now = entry_should_be_refused();
    if (refuse_now != refused) {
      refused = refuse_now;
      if (refused) {
        hal_boot_entry_disable();
        print("entry DISABLED -- a matched sequence will be refused\r\n");
      } else {
        hal_boot_entry_enable();
        print("entry enabled\r\n");
      }
    }

    hal_gpio_toggle(LED_BUILTIN);
    hal_delay_ms(refused ? 100u : 500u); /* fast blink while refusing */
  }
}
