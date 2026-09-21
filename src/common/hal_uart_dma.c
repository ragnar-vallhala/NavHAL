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
 * @file common/hal_uart_dma.c
 * @brief Shared UART-over-DMA layer.
 *
 * @details
 * The four public @c hal_uart_*_dma entry points, written once. Both STM32
 * families previously carried their own copies of this logic, and the F7 copy
 * was missing @c hal_uart_dma_rx_index entirely; a single implementation
 * cannot diverge that way.
 *
 * The vendor backend supplies only the DMA wiring and the USART request-line
 * bit (::hal_uart_dma_ops_t); the descriptor, the re-arm path and the
 * circular-index arithmetic are the same on any controller.
 */

#include "common/hal_uart.h"
#include "internal/hal_uart_dma_ops.h"

#if NAVHAL_CONFIG_DRV_DMA && NAVHAL_CONFIG_DRV_UART_DMA

#include "common/hal_dma.h"
#include "common/hal_interrupt.h"

#include <stddef.h>

/* hal_uart_t enumerators are USART numbers, not dense indices (F4 uses 1, 2,
 * 6), so these are sparse lookup tables sized by the largest id rather than by
 * a count. Cheap: a handful of bytes. */
#define UART_SLOTS 8u

/** Per-UART bookkeeping the public API needs but the hardware does not hold. */
static uint8_t s_tx_armed[UART_SLOTS];
static uint16_t s_rx_len[UART_SLOTS];

static hal_dma_binding_t s_override[UART_SLOTS][2];
static bool s_has_override[UART_SLOTS][2];

hal_status_t hal_uart_dma_get_binding(hal_uart_t uart, bool tx,
                                      hal_dma_binding_t *out) {
  if (out == NULL || (unsigned)uart >= UART_SLOTS)
    return HAL_ERR_INVALID_ARG;

  if (s_has_override[uart][tx ? 1u : 0u]) {
    *out = s_override[uart][tx ? 1u : 0u];
    return HAL_OK;
  }
  return _hal_uart_dma_ops.binding(uart, tx, out);
}

hal_status_t hal_uart_dma_set_binding(hal_uart_t uart, bool tx,
                                      const hal_dma_binding_t *binding) {
  if ((unsigned)uart >= UART_SLOTS)
    return HAL_ERR_INVALID_ARG;

  /* NULL clears the override and restores the hardware default. */
  if (binding == NULL) {
    s_has_override[uart][tx ? 1u : 0u] = false;
    return HAL_OK;
  }

  /* Refuse an override for a UART this port has no mapping for. */
  hal_dma_binding_t probe;
  hal_status_t st = _hal_uart_dma_ops.binding(uart, tx, &probe);
  if (st != HAL_OK)
    return st;

  s_override[uart][tx ? 1u : 0u] = *binding;
  s_has_override[uart][tx ? 1u : 0u] = true;
  return HAL_OK;
}

static hal_status_t _descriptor(hal_uart_t uart, bool tx, uint32_t mem,
                                uint16_t count, hal_dma_config_t *out) {
  hal_dma_binding_t b;
  hal_status_t st = hal_uart_dma_get_binding(uart, tx, &b);
  if (st != HAL_OK)
    return st;

  hal_dma_config_t cfg = {
      .controller = b.controller,
      .stream = b.stream,
      .channel = b.channel,
      .direction = tx ? HAL_DMA_DIR_M2P : HAL_DMA_DIR_P2M,
      .src_addr = tx ? mem : b.periph_addr,
      .dst_addr = tx ? b.periph_addr : mem,
      .data_count = count,
      .src_inc = tx ? 1u : 0u,
      .dst_inc = tx ? 0u : 1u,
      .data_width = HAL_DMA_DATA_WIDTH_8,
      .priority = tx ? HAL_DMA_PRIORITY_HIGH : HAL_DMA_PRIORITY_MEDIUM,
      .circular = tx ? 0u : 1u,
  };
  *out = cfg;
  return HAL_OK;
}

hal_status_t hal_uart_write_dma(hal_uart_t uart, const uint8_t *data,
                                uint16_t length) {
  if (data == NULL || length == 0u || (unsigned)uart >= UART_SLOTS)
    return HAL_ERR_INVALID_ARG;

  hal_dma_config_t cfg;
  hal_status_t st = _descriptor(uart, true, (uint32_t)data, length, &cfg);
  if (st != HAL_OK)
    return st;

  st = _hal_uart_dma_ops.set_request(uart, true, true);
  if (st != HAL_OK)
    return st;

  if (!s_tx_armed[uart]) {
    st = hal_dma_init(&cfg);
    if (st != HAL_OK)
      return st;
    s_tx_armed[uart] = 1u;
  } else {
    /* Stream already configured: re-point it instead of reinitialising, which
     * also waits out any transfer still in flight. */
    st = hal_dma_set_memory(&cfg, (uint32_t)data, length);
    if (st != HAL_OK)
      return st;
  }

  hal_dma_binding_t b;
  if (hal_uart_dma_get_binding(uart, true, &b) == HAL_OK)
    hal_interrupt_enable(b.irq);

  hal_dma_clear_flags(&cfg);
  return hal_dma_start(&cfg);
}

hal_status_t hal_uart_init_dma_rx(hal_uart_t uart, uint8_t *buffer,
                                  uint16_t length) {
  if (buffer == NULL || length == 0u || (unsigned)uart >= UART_SLOTS)
    return HAL_ERR_INVALID_ARG;

  hal_dma_config_t cfg;
  hal_status_t st = _descriptor(uart, false, (uint32_t)buffer, length, &cfg);
  if (st != HAL_OK)
    return st;

  st = _hal_uart_dma_ops.set_request(uart, false, true);
  if (st != HAL_OK)
    return st;

  st = hal_dma_init(&cfg);
  if (st != HAL_OK)
    return st;

  s_rx_len[uart] = length;
  return hal_dma_start(&cfg);
}

hal_status_t hal_uart_dma_rx_index(hal_uart_t uart, uint16_t *out_index) {
  if (out_index == NULL || (unsigned)uart >= UART_SLOTS)
    return HAL_ERR_INVALID_ARG;

  uint16_t len = s_rx_len[uart];
  if (len == 0u)
    return HAL_ERR_INVALID_ARG; /* RX DMA not configured for this UART */

  hal_dma_config_t cfg;
  hal_status_t st = _descriptor(uart, false, 0u, len, &cfg);
  if (st != HAL_OK)
    return st;

  /* The controller counts down to zero as it fills the ring, so the next
   * write position is the distance already travelled. */
  uint16_t remaining = 0u;
  st = hal_dma_remaining(&cfg, &remaining);
  if (st != HAL_OK)
    return st;

  *out_index = (uint16_t)(len - remaining);
  return HAL_OK;
}

hal_status_t hal_uart_write_string_dma(hal_uart_t uart, const char *s) {
  if (s == NULL)
    return HAL_ERR_INVALID_ARG;

  uint16_t len = 0u;
  while (s[len] != '\0')
    len++;

  return hal_uart_write_dma(uart, (const uint8_t *)s, len);
}

#endif /* NAVHAL_CONFIG_DRV_DMA && NAVHAL_CONFIG_DRV_UART_DMA */
