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
 * @brief UART <-> Ethernet chat bridge on the Nucleo-F767ZI.
 *
 * @details
 * Bridges the USART3 console (ST-LINK VCP, 9600 8N1) and raw Ethernet L2:
 *   - a line typed on the console (terminated by Enter) is sent as one Ethernet
 *     frame to the peer, and
 *   - a frame received from the peer is printed to the console.
 *
 * Pair it with samples/cortex-m/30_hal_eth_bridge/chat.py on the host: type in
 * `minicom` (the board's VCP) and it appears in the Python window, and vice
 * versa. Raw L2 only — no IP stack.
 *
 * Frame layout after the 14-byte Ethernet header: a 2-byte big-endian payload
 * length followed by the text. The MAC pads short frames to 60 bytes, so the
 * length field is what tells the receiver the real text size.
 */

#include "navhal.h"

#define CONSOLE HAL_UART_3    /* USART3 = ST-LINK VCP. */
#define ETH_CHAT_TYPE 0x88B5U /* local/experimental EtherType. */
#define LINE_MAX 256

static const uint8_t BOARD_MAC[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
static const uint8_t BCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static uint8_t s_tx[HAL_ETH_MAX_FRAME_LEN];
static uint8_t s_rx[HAL_ETH_MAX_FRAME_LEN];
static uint8_t s_line[LINE_MAX];
static uint16_t s_linelen;

/* Build and transmit one chat frame carrying s_line[0..len). */
static void send_line(uint16_t len) {
  for (int i = 0; i < 6; i++)
    s_tx[i] = BCAST_MAC[i]; /* broadcast so the host's raw socket sees it. */
  for (int i = 0; i < 6; i++)
    s_tx[6 + i] = BOARD_MAC[i];
  s_tx[12] = (uint8_t)(ETH_CHAT_TYPE >> 8);
  s_tx[13] = (uint8_t)(ETH_CHAT_TYPE & 0xFF);
  s_tx[14] = (uint8_t)(len >> 8);
  s_tx[15] = (uint8_t)(len & 0xFF);
  for (uint16_t i = 0; i < len; i++)
    s_tx[16 + i] = s_line[i];

  uint16_t total = (uint16_t)(16 + len);
  if (total < HAL_ETH_MIN_FRAME_LEN) {
    for (uint16_t i = total; i < HAL_ETH_MIN_FRAME_LEN; i++)
      s_tx[i] = 0;
    total = HAL_ETH_MIN_FRAME_LEN;
  }
  hal_eth_send(s_tx, total);
}

int main(void) {
  hal_pll_config_t pll = {.input_src = HAL_CLOCK_SOURCE_HSI,
                          .pll_m = 8,
                          .pll_n = 100,
                          .pll_p = 2,
                          .pll_q = 5}; /* 100 MHz: ETH needs HCLK >= 25 MHz. */
  hal_clock_config_t clk = {.source = HAL_CLOCK_SOURCE_PLL};
  hal_clock_init(&clk, &pll);

  hal_uart_init(CONSOLE, &(hal_uart_config_t){.baudrate = 9600});
  hal_uart_print(CONSOLE, "\r\n[bridge] UART<->Ethernet chat. Type + Enter.\r\n");

  hal_eth_config_t cfg = {
      .interface = HAL_ETH_IFACE_RMII,
      .phy_address = 0,
      .auto_negotiation = true,
      .promiscuous = false,
  };
  for (int i = 0; i < 6; i++)
    cfg.mac_addr[i] = BOARD_MAC[i];
  if (hal_eth_init(&cfg) != HAL_OK) {
    hal_uart_print(CONSOLE, "[bridge] eth init failed\r\n");
    for (;;)
      ;
  }
  hal_eth_start();

  for (;;) {
    /* Console -> Ethernet: accumulate a line, send it on Enter. The terminal
     * (minicom/screen) echoes typed characters locally, so the bridge must not
     * echo them too — doing so would double every character. */
    if (hal_uart_available(CONSOLE)) {
      char c = hal_uart_read_char(CONSOLE);
      if (c == '\r' || c == '\n') {
        if (s_linelen > 0) {
          send_line(s_linelen);
          s_linelen = 0;
        }
      } else if (c == 0x08 || c == 0x7F) { /* backspace / delete. */
        if (s_linelen > 0)
          s_linelen--;
      } else if (s_linelen < LINE_MAX) {
        s_line[s_linelen++] = (uint8_t)c;
      }
    }

    /* Ethernet -> console: print the payload of any received chat frame. */
    uint16_t n = 0;
    if (hal_eth_receive(s_rx, sizeof(s_rx), &n) == HAL_OK && n >= 16) {
      if (s_rx[12] == (uint8_t)(ETH_CHAT_TYPE >> 8) &&
          s_rx[13] == (uint8_t)(ETH_CHAT_TYPE & 0xFF)) {
        uint16_t plen = (uint16_t)((s_rx[14] << 8) | s_rx[15]);
        if (plen > (uint16_t)(n - 16))
          plen = (uint16_t)(n - 16);
        hal_uart_print(CONSOLE, "[net] ");
        for (uint16_t i = 0; i < plen; i++)
          hal_uart_write_char(CONSOLE, (char)s_rx[16 + i]);
        hal_uart_print(CONSOLE, "\r\n");
      }
    }
  }
}
