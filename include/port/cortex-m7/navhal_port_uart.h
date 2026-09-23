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
 * @file port/cortex-m4/navhal_port_uart.h
 * @brief Cortex-M4 / STM32F4 UART port header.
 *
 * @details
 * The public UART API lives in @c common/hal_uart.h, which includes this
 * header. This file carries the DMA-backed UART prototypes (available only
 * when the DMA backend is enabled) and the deprecated-name shim.
 */

#ifndef NAVHAL_PORT_UART_H
#define NAVHAL_PORT_UART_H

#include "common/hal_uart.h"

#include <stdbool.h>
#include "navhal_port_config.h"
#include "family/uart_reg.h"
/* hal_dma_binding_t, for the DMA binding accessors below. */
#include "navhal_port_dma.h"


#ifdef __cplusplus
extern "C" {
#endif

/* NAVHAL_CONFIG_DRV_UART_DMA is force-included (from navhal_target.h) and lets
   the UART driver's DMA paths be disabled independently of other DMA users. */

/* -------------------------------------------------------------------------- *
 * DMA-backed UART API — available only when the DMA backend is enabled.
 * -------------------------------------------------------------------------- */
#if NAVHAL_CONFIG_DRV_DMA && NAVHAL_CONFIG_DRV_UART_DMA

/**
 * @brief The DMA wiring this UART and direction will use.
 *
 * The reference-manual default, or the override installed by
 * ::hal_uart_dma_set_binding.
 *
 * @param uart UART instance.
 * @param tx   true for the transmit request, false for receive.
 * @param out  Receives the binding; must not be NULL.
 */
hal_status_t hal_uart_dma_get_binding(hal_uart_t uart, bool tx,
                                      hal_dma_binding_t *out);

/**
 * @brief Override the DMA wiring for a UART and direction.
 *
 * Several USART requests have a second stream on this part -- USART1_RX is
 * DMA2 stream 2 or 5, USART6_RX is stream 1 or 2, USART6_TX is 6 or 7 -- so
 * the default is not the only option when something else holds that stream.
 * Pass NULL to restore it.
 */
hal_status_t hal_uart_dma_set_binding(hal_uart_t uart, bool tx,
                                      const hal_dma_binding_t *binding);

hal_status_t hal_uart_write_dma(hal_uart_t uart, const uint8_t *data,
                                uint16_t length);
/** @brief Set up a UART for DMA-based circular reception. */
hal_status_t hal_uart_init_dma_rx(hal_uart_t uart, uint8_t *buffer,
                                  uint16_t length);

/**
 * @brief Current circular RX write index for a UART set up via
 *        ::hal_uart_init_dma_rx (i.e. @c length-NDTR on the correct stream).
 *
 * Lets consumers track how far the DMA has filled the ring without touching
 * DMA registers themselves. Valid range is [0, length).
 *
 * @param uart       UART previously initialised for DMA RX.
 * @param out_index  Receives the next-write index.
 * @return HAL_OK, or HAL_ERR_INVALID_ARG if RX DMA isn't configured / NULL out.
 */
hal_status_t hal_uart_dma_rx_index(hal_uart_t uart, uint16_t *out_index);

/**
 * @brief Make the CPU's view of an RX ring span match what the DMA wrote.
 *
 * Invalidates @p len bytes from @p from in the ring registered with
 * ::hal_uart_init_dma_rx, so a subsequent read returns received data rather
 * than a cached copy from before the transfer. A wrapped span is handled.
 *
 * Call it before reading the ring, on the span between the last consumed index
 * and ::hal_uart_dma_rx_index. Without a data cache it does nothing, so a
 * caller that wants to be portable calls it unconditionally.
 *
 * @param uart UART previously initialised for DMA RX.
 * @param from Byte offset into the ring.
 * @param len  Number of bytes to make visible.
 * @return ::HAL_OK, or ::HAL_ERR_INVALID_ARG for an unknown UART, a ring that
 *         was never registered, or a span that does not fit it.
 */
hal_status_t hal_uart_dma_rx_sync(hal_uart_t uart, uint16_t from, uint16_t len);
/** @brief Transmit a null-terminated string using DMA. */
hal_status_t hal_uart_write_string_dma(hal_uart_t uart, const char *s);

#endif /* NAVHAL_CONFIG_DRV_DMA && NAVHAL_CONFIG_DRV_UART_DMA */

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Deprecated pre-standardization UART names — retained as a backward-compat alias. */
#include "compat/uart_compat.h"

#endif /* NAVHAL_PORT_UART_H */
