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

#include "navhal_port_config.h"
#include "family/dma_reg.h"
#include "navhal.h"

#define BUF_SIZE 256

/* Circular DMA RX buffers, read live via NDTR. Cache-line aligned so a D-cache
 * invalidate stays within the buffer. NOTE: on a cache-on build a circular RX
 * buffer read live is only coherent if placed in DTCM (uncached, DMA-reachable)
 * or invalidated before each read — see hal_uart_init_dma_rx. This board (F401,
 * no D-cache) needs neither; the alignment documents the contract. */
uint8_t u2_rx_buf[BUF_SIZE] NAVHAL_DMA_ALIGN;
uint8_t u6_rx_buf[BUF_SIZE] NAVHAL_DMA_ALIGN;

uint16_t u2_head = 0;
uint16_t u6_head = 0;

int main(void) {
  hal_timebase_init(1000);

  /* Initialize HAL_UART_2 and HAL_UART_6 at 115200 bps */
  hal_uart_init(HAL_UART_2, &(hal_uart_config_t){.baudrate=115200});
  hal_uart_init(HAL_UART_6, &(hal_uart_config_t){.baudrate=115200});

#if NAVHAL_CONFIG_DRV_DMA && NAVHAL_CONFIG_DRV_UART_DMA
  /* Start DMA circular reception for both UARTs */
  hal_uart_init_dma_rx(HAL_UART_2, u2_rx_buf, BUF_SIZE);
  hal_uart_init_dma_rx(HAL_UART_6, u6_rx_buf, BUF_SIZE);

  /* Optional start message */
  hal_uart_write_string_dma(HAL_UART_2, "Bridge Started: HAL_UART_2 <-> HAL_UART_6\r\n");
  hal_uart_write_string_dma(HAL_UART_6, "Bridge Started: HAL_UART_6 <-> HAL_UART_2\r\n");

  while (1) {
    /* Check HAL_UART_2 RX buffer via DMA NDTR */
    uint16_t u2_tail = BUF_SIZE - DMA1->STREAM[5].NDTR;
    if (u2_tail != u2_head) {
      if (u2_tail > u2_head) {
        hal_uart_write_dma(HAL_UART_6, &u2_rx_buf[u2_head], u2_tail - u2_head);
      } else {
        hal_uart_write_dma(HAL_UART_6, &u2_rx_buf[u2_head], BUF_SIZE - u2_head);
        if (u2_tail > 0) {
          hal_uart_write_dma(HAL_UART_6, &u2_rx_buf[0], u2_tail);
        }
      }
      u2_head = u2_tail;
    }

    /* Check HAL_UART_6 RX buffer via DMA NDTR */
    uint16_t u6_tail = BUF_SIZE - DMA2->STREAM[1].NDTR;
    if (u6_tail != u6_head) {
      if (u6_tail > u6_head) {
        hal_uart_write_dma(HAL_UART_2, &u6_rx_buf[u6_head], u6_tail - u6_head);
      } else {
        hal_uart_write_dma(HAL_UART_2, &u6_rx_buf[u6_head], BUF_SIZE - u6_head);
        if (u6_tail > 0) {
          hal_uart_write_dma(HAL_UART_2, &u6_rx_buf[0], u6_tail);
        }
      }
      u6_head = u6_tail;
    }
  }
#else
  /* Fallback if DMA is not enabled */
  while (1) {
    if (hal_uart_available(HAL_UART_2)) {
      hal_uart_write_char(HAL_UART_6, hal_uart_read_char(HAL_UART_2));
    }
    if (hal_uart_available(HAL_UART_6)) {
      hal_uart_write_char(HAL_UART_2, hal_uart_read_char(HAL_UART_6));
    }
  }
#endif

  return 0;
}
