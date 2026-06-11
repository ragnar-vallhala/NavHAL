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
#include "navhal_port_config.h"
#include "family/uart_reg.h"


#ifdef __cplusplus
extern "C" {
#endif

/* The _UART_BACKEND_DMA selector is derived in navhal_port_config.h from
   NAVHAL_HAS_UART_DMA so that the UART driver's DMA paths can be disabled
   independently of other DMA users. */

/* -------------------------------------------------------------------------- *
 * IDLE-line interrupt → callback (DMA-independent).
 *
 * Lets a consumer wake on the inter-frame gap of a byte-oriented protocol
 * (SBUS/iBUS/etc.) instead of polling. Typically paired with circular DMA RX:
 * arm DMA, then attach an idle callback that signals a task to drain the ring.
 * -------------------------------------------------------------------------- */

/**
 * @brief Enable the UART IDLE-line interrupt and route it to @p callback.
 *
 * The IDLE flag is cleared inside the driver before @p callback runs. The line
 * is enabled at a maskable (BASEPRI-managed) NVIC priority, so @p callback may
 * call an RTOS @c *_from_isr primitive (e.g. give a semaphore).
 *
 * @param uart      Target UART.
 * @param callback  Invoked from ISR context on each detected idle frame-gap.
 * @return HAL_OK, or HAL_ERR_INVALID_ARG for an unknown UART / NULL callback.
 */
hal_status_t hal_uart_attach_idle_callback(hal_uart_t uart,
                                           void (*callback)(void));

/** @brief Disable the IDLE-line interrupt and clear the registered callback. */
hal_status_t hal_uart_detach_idle_callback(hal_uart_t uart);

/* -------------------------------------------------------------------------- *
 * DMA-backed UART API — available only when the DMA backend is enabled.
 * -------------------------------------------------------------------------- */
#if defined(_DMA_ENABLED) && defined(_UART_BACKEND_DMA)

/** @brief Transmit a byte buffer using DMA (buffer must stay valid). */
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
/** @brief Transmit a null-terminated string using DMA. */
hal_status_t hal_uart_write_string_dma(hal_uart_t uart, const char *s);

#endif /* _DMA_ENABLED && _UART_BACKEND_DMA */

#ifdef __cplusplus
} /* extern "C" */
#endif

/* Deprecated pre-standardization UART names — retained as a backward-compat alias. */
#include "compat/uart_compat.h"

#endif /* NAVHAL_PORT_UART_H */
