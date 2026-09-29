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
#include "board.h"
#include "navhal.h"

/* Kconfig keeps this sample out of the menu on a board whose BOARD_AUX_UART is
 * already committed; this catches -DSAMPLE= naming it anyway. */
#if !NAVHAL_CONFIG_BOARD_AUX_UART_SPARE
#error "this sample drives two UARTs, and BOARD_AUX_UART is not spare on this board"
#endif

#define BUF_SIZE 256

/* Circular DMA RX buffers. Cache-line aligned so a D-cache invalidate stays
 * within the buffer: on a cache-on build a circular RX buffer read live is only
 * coherent in DTCM (uncached, DMA-reachable) or if invalidated before each read
 * -- see hal_uart_init_dma_rx. A part without a D-cache needs neither, and the
 * alignment documents the contract either way. */
uint8_t con_rx_buf[BUF_SIZE] NAVHAL_DMA_ALIGN;
uint8_t aux_rx_buf[BUF_SIZE] NAVHAL_DMA_ALIGN;

uint16_t con_head = 0;
uint16_t aux_head = 0;

int main(void) {
  hal_timebase_init(1000);

  hal_uart_init(BOARD_CONSOLE_UART, &(hal_uart_config_t){.baudrate=115200});
  hal_uart_init(BOARD_AUX_UART, &(hal_uart_config_t){.baudrate=115200});

#if NAVHAL_CONFIG_DRV_DMA && NAVHAL_CONFIG_DRV_UART_DMA
  /* Start DMA circular reception for both UARTs */
  hal_uart_init_dma_rx(BOARD_CONSOLE_UART, con_rx_buf, BUF_SIZE);
  hal_uart_init_dma_rx(BOARD_AUX_UART, aux_rx_buf, BUF_SIZE);

  hal_uart_write_string_dma(BOARD_CONSOLE_UART, "Bridge started: console -> aux\r\n");
  hal_uart_write_string_dma(BOARD_AUX_UART, "Bridge started: aux -> console\r\n");

  while (1) {
    /* How far the DMA has filled each RX ring. The driver knows which stream
     * belongs to which UART; reading NDTR here would hardcode that mapping. */
    uint16_t con_tail = con_head;
    (void)hal_uart_dma_rx_index(BOARD_CONSOLE_UART, &con_tail);
    if (con_tail != con_head) {
      if (con_tail > con_head) {
        hal_uart_write_dma(BOARD_AUX_UART, &con_rx_buf[con_head], con_tail - con_head);
      } else {
        hal_uart_write_dma(BOARD_AUX_UART, &con_rx_buf[con_head], BUF_SIZE - con_head);
        if (con_tail > 0) {
          hal_uart_write_dma(BOARD_AUX_UART, &con_rx_buf[0], con_tail);
        }
      }
      con_head = con_tail;
    }

    uint16_t aux_tail = aux_head;
    (void)hal_uart_dma_rx_index(BOARD_AUX_UART, &aux_tail);
    if (aux_tail != aux_head) {
      if (aux_tail > aux_head) {
        hal_uart_write_dma(BOARD_CONSOLE_UART, &aux_rx_buf[aux_head], aux_tail - aux_head);
      } else {
        hal_uart_write_dma(BOARD_CONSOLE_UART, &aux_rx_buf[aux_head], BUF_SIZE - aux_head);
        if (aux_tail > 0) {
          hal_uart_write_dma(BOARD_CONSOLE_UART, &aux_rx_buf[0], aux_tail);
        }
      }
      aux_head = aux_tail;
    }
  }
#else
  /* Fallback if DMA is not enabled */
  while (1) {
    if (hal_uart_available(BOARD_CONSOLE_UART)) {
      hal_uart_write_char(BOARD_AUX_UART, hal_uart_read_char(BOARD_CONSOLE_UART));
    }
    if (hal_uart_available(BOARD_AUX_UART)) {
      hal_uart_write_char(BOARD_CONSOLE_UART, hal_uart_read_char(BOARD_AUX_UART));
    }
  }
#endif

  return 0;
}
