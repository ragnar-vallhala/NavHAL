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
 * @brief Ethernet L2 echo server on the Nucleo-F767ZI.
 *
 * @details
 * Reflects every received frame back to its sender: swaps the source/destination
 * MACs and retransmits the same EtherType + payload. Pair it with
 * samples/cortex-m/31_hal_eth_echo/bench.py on the host to ramp the offered load
 * and measure round-trip bandwidth.
 *
 * The echo loop prints nothing after boot — console traffic at 9600 baud would
 * throttle the loop and skew the measurement; the host tool does the accounting.
 */

#include "navhal.h"

#define CONSOLE HAL_UART_3
#define ETH_ECHO_TYPE 0x88B5U

static const uint8_t BOARD_MAC[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};

static uint8_t s_rx[HAL_ETH_MAX_FRAME_LEN];
static uint8_t s_tx[HAL_ETH_MAX_FRAME_LEN];

int main(void) {
#if NAVHAL_CONFIG_DRV_CACHE
  hal_icache_enable();
  hal_dcache_enable(); /* exercises the ETH DMA path under the L1 D-cache */
#endif
  hal_pll_config_t pll = {.input_src = HAL_CLOCK_SOURCE_HSI,
                          .pll_m = 8,
                          .pll_n = 100,
                          .pll_p = 2,
                          .pll_q = 5}; /* 100 MHz: ETH needs HCLK >= 25 MHz. */
  hal_clock_config_t clk = {.source = HAL_CLOCK_SOURCE_PLL};
  clk.pll = pll;
  hal_clock_init(&clk);

  hal_uart_init(CONSOLE, &(hal_uart_config_t){.baudrate = 9600});
  hal_uart_print(CONSOLE, "\r\n[echo] Ethernet echo server up. Run bench.py.\r\n");

  hal_eth_config_t cfg = {
      .interface = HAL_ETH_IFACE_RMII,
      .phy_address = 0,
      .auto_negotiation = true,
      .promiscuous = false,
  };
  for (int i = 0; i < 6; i++)
    cfg.mac_addr[i] = BOARD_MAC[i];
  if (hal_eth_init(&cfg) != HAL_OK) {
    hal_uart_print(CONSOLE, "[echo] eth init failed\r\n");
    for (;;)
      ;
  }
  hal_eth_start();

  for (;;) {
    uint16_t n = 0;
    if (hal_eth_receive(s_rx, sizeof(s_rx), &n) == HAL_OK && n >= 14) {
      if (s_rx[12] != (uint8_t)(ETH_ECHO_TYPE >> 8) ||
          s_rx[13] != (uint8_t)(ETH_ECHO_TYPE & 0xFF))
        continue; /* not ours. */

      /* Reflect: destination = original sender, source = us, body unchanged. */
      for (int i = 0; i < 6; i++)
        s_tx[i] = s_rx[6 + i];
      for (int i = 0; i < 6; i++)
        s_tx[6 + i] = BOARD_MAC[i];
      for (uint16_t i = 12; i < n; i++)
        s_tx[i] = s_rx[i];

      /* Retry while the TX ring is momentarily full so the echo isn't dropped
       * under load; drop only if the frame is somehow undersized. */
      while (hal_eth_send(s_tx, n) == HAL_ERR_BUSY)
        ;
    }
  }
}
