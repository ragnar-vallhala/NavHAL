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
 * @brief Ethernet bring-up on the Nucleo-F767ZI.
 *
 * @details
 * Brings up the MAC + RMII PHY, reports link state on the USART3 console
 * (ST-LINK VCP, 9600 8N1), then once the link is up periodically broadcasts a
 * small tagged frame and prints any frame it receives. Cable the RJ45 to a host
 * and watch the frames with, e.g., `tcpdump -i <iface> ether proto 0x88b5 -e`.
 *
 * The MAC data path needs HCLK >= 25 MHz, so the PLL is brought up first.
 */

#include "navhal.h"

#define ETH_CONSOLE HAL_UART_3 /* USART3 = ST-LINK VCP on the Nucleo-144. */
#define ETH_ETHERTYPE 0x88B5U  /* local/experimental EtherType. */

static uint8_t s_mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
static uint8_t s_tx[64];
static uint8_t s_rx[HAL_ETH_MAX_FRAME_LEN];

static void print_u32(uint32_t v) {
  char buf[11];
  int i = 10;
  buf[i] = '\0';
  if (v == 0)
    buf[--i] = '0';
  while (v && i > 0) {
    buf[--i] = (char)('0' + (v % 10));
    v /= 10;
  }
  hal_uart_print(ETH_CONSOLE, &buf[i]);
}

static void busy_wait(volatile uint32_t n) {
  while (n--)
    __asm__ volatile("nop");
}

int main(void) {
  hal_pll_config_t pll = {.input_src = HAL_CLOCK_SOURCE_HSI,
                          .pll_m = 8,
                          .pll_n = 100,
                          .pll_p = 2,
                          .pll_q = 5}; /* HSI 16 MHz -> 100 MHz SYSCLK/HCLK. */
  hal_clock_config_t clk = {.source = HAL_CLOCK_SOURCE_PLL};
  clk.pll = pll;
  hal_clock_init(&clk);

  hal_uart_init(ETH_CONSOLE, &(hal_uart_config_t){.baudrate = 9600});
  hal_uart_print(ETH_CONSOLE, "\r\n[eth] bring-up, hclk=");
  print_u32(hal_clock_get_ahbclk());
  hal_uart_print(ETH_CONSOLE, " Hz\r\n");

  hal_eth_config_t cfg = {
      .interface = HAL_ETH_IFACE_RMII,
      .phy_address = 0,
      .auto_negotiation = true,
      .promiscuous = false,
  };
  for (int i = 0; i < 6; i++)
    cfg.mac_addr[i] = s_mac[i];

  if (hal_eth_init(&cfg) != HAL_OK) {
    hal_uart_print(ETH_CONSOLE, "[eth] init failed\r\n");
    for (;;)
      ;
  }
  hal_eth_start();
  hal_uart_print(ETH_CONSOLE, "[eth] started, waiting for link...\r\n");

  bool was_up = false;
  uint32_t tx_count = 0;

  for (;;) {
    hal_eth_link_t link;
    hal_eth_get_link(&link);

    if (link.up != was_up) {
      was_up = link.up;
      if (link.up) {
        hal_uart_print(ETH_CONSOLE, "[eth] link UP ");
        print_u32(link.speed == HAL_ETH_SPEED_100M ? 100 : 10);
        hal_uart_print(ETH_CONSOLE,
                       link.duplex == HAL_ETH_FULL_DUPLEX ? "M full\r\n"
                                                          : "M half\r\n");
      } else {
        hal_uart_print(ETH_CONSOLE, "[eth] link DOWN\r\n");
      }
    }

    /* Drain any received frames. */
    uint16_t rx_len = 0;
    if (hal_eth_receive(s_rx, sizeof(s_rx), &rx_len) == HAL_OK && rx_len > 0) {
      hal_uart_print(ETH_CONSOLE, "[eth] rx len=");
      print_u32(rx_len);
      hal_uart_print(ETH_CONSOLE, "\r\n");
    }

    if (link.up) {
      /* Broadcast a tagged frame roughly once per second. */
      for (int i = 0; i < 6; i++)
        s_tx[i] = 0xFF; /* destination: broadcast. */
      for (int i = 0; i < 6; i++)
        s_tx[6 + i] = s_mac[i];
      s_tx[12] = (uint8_t)(ETH_ETHERTYPE >> 8);
      s_tx[13] = (uint8_t)(ETH_ETHERTYPE & 0xFF);
      const char *tag = "NAVHAL-ETH";
      int p = 14;
      for (const char *c = tag; *c; c++)
        s_tx[p++] = (uint8_t)*c;
      while (p < 60)
        s_tx[p++] = 0;

      if (hal_eth_send(s_tx, 60) == HAL_OK) {
        hal_uart_print(ETH_CONSOLE, "[eth] tx ");
        print_u32(++tx_count);
        hal_uart_print(ETH_CONSOLE, "\r\n");
      }
      busy_wait(20000000u); /* ~1 s at 100 MHz. */
    } else {
      busy_wait(2000000u);
    }
  }
}
