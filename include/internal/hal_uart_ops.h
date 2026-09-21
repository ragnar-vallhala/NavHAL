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
 * @brief HAL-internal UART vendor-backend interface (hardware primitives).
 *
 * @details
 * Not part of the public API — application code includes @c common/hal_uart.h.
 *
 * This table is deliberately *minimal*: it exposes only the operations that
 * actually touch UART hardware. Everything expressible on top of them — buffer
 * writes, the integer/unsigned/float/string formatters, and delimited reads —
 * is implemented **once** in the shared public layer @c src/common/hal_uart.c,
 * not re-coded in every vendor driver. A new UART port supplies these five
 * primitives and inherits the entire formatted-I/O API for free.
 *
 * See @c internal/hal_gpio_ops.h for the embedded-table / LTO rationale.
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

/** @brief Per-backend UART hardware primitives. */
typedef struct {
  /** Configure the UART (8N1, TX+RX). NULL @p cfg rejected upstream. */
  hal_status_t (*init)(hal_uart_t uart, const hal_uart_config_t *cfg);
  /** Enable/disable the UART's RX/TX interrupts (peripheral + controller). */
  hal_status_t (*enable_interrupt)(hal_uart_t uart, uint8_t rx_en,
                                   uint8_t tx_en);
  /** Transmit one character (blocking). The single TX primitive — every
   *  higher-level write in the common layer is built from this. */
  hal_status_t (*write_char)(hal_uart_t uart, char c);
  /** Receive one character (blocking). The single RX primitive. */
  char (*read_char)(hal_uart_t uart);
  /** Non-blocking check for a pending received byte. */
  bool (*available)(hal_uart_t uart);
} hal_uart_ops_t;

/** @brief The active port's UART primitives (defined by one backend). */
extern const hal_uart_ops_t _hal_uart_ops;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NAVHAL_INTERNAL_HAL_UART_OPS_H */
