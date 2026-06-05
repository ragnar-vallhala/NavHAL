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
 * @file internal/hal_uart_ops.h
 * @brief HAL-internal UART vendor-backend interface (the driver vtable).
 *
 * @details
 * Not part of the public API — application code includes @c common/hal_uart.h.
 * Declares the per-backend operations table the shared public layer
 * (@c src/common/hal_uart.c) dispatches through. Only the portable @c hal_uart_*
 * API is in the table; port-specific extensions (e.g. the STM32 DMA transmit
 * helpers in @c navhal_port_uart.h) stay outside it. See @c internal/hal_gpio_ops.h
 * for the embedded-table rationale.
 */

#ifndef NAVHAL_INTERNAL_HAL_UART_OPS_H
#define NAVHAL_INTERNAL_HAL_UART_OPS_H

#include "common/hal_uart.h"
#include "common/hal_status.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Per-backend UART operations table (portable API only). */
typedef struct {
  /** Backend for ::hal_uart_init. NULL @p cfg rejected upstream. */
  hal_status_t (*init)(hal_uart_t uart, const hal_uart_config_t *cfg);
  /** Backend for ::hal_uart_enable_interrupt. */
  hal_status_t (*enable_interrupt)(hal_uart_t uart, uint8_t rx_en,
                                   uint8_t tx_en);
  /** Backend for ::hal_uart_write. NULL @p data rejected upstream. */
  hal_status_t (*write)(hal_uart_t uart, const uint8_t *data, uint16_t length);
  /** Backend for ::hal_uart_write_char. */
  hal_status_t (*write_char)(hal_uart_t uart, char c);
  /** Backend for ::hal_uart_write_int. */
  hal_status_t (*write_int)(hal_uart_t uart, int32_t num);
  /** Backend for ::hal_uart_write_uint. */
  hal_status_t (*write_uint)(hal_uart_t uart, uint32_t num);
  /** Backend for ::hal_uart_write_float. */
  hal_status_t (*write_float)(hal_uart_t uart, float num);
  /** Backend for ::hal_uart_write_string. NULL @p s rejected upstream. */
  hal_status_t (*write_string)(hal_uart_t uart, const char *s);
  /** Backend for ::hal_uart_read_char. */
  char (*read_char)(hal_uart_t uart);
  /** Backend for ::hal_uart_available. */
  bool (*available)(hal_uart_t uart);
  /** Backend for ::hal_uart_read_until. NULL buffer / zero maxlen rejected upstream. */
  uint32_t (*read_until)(hal_uart_t uart, char *buffer, uint32_t maxlen,
                         char delimiter);
} hal_uart_ops_t;

/** @brief The active port's UART operations table (defined by one backend). */
extern const hal_uart_ops_t _hal_uart_ops;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_INTERNAL_HAL_UART_OPS_H */
