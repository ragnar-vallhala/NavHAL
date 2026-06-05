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
 * @file src/common/hal_uart.c
 * @brief Shared public UART layer: validate, then dispatch to the vendor vtable.
 *
 * @details
 * The one implementation of the portable @c hal_uart_* API. It hoists the
 * documented NULL-argument checks (config, data buffer, string, read buffer)
 * that every backend duplicated. Instance validity (which UART ids exist) is
 * vendor-specific, so it stays in the backend.
 */

#include "common/hal_uart.h"
#include "internal/hal_uart_ops.h"

#include <stddef.h>

hal_status_t hal_uart_init(hal_uart_t uart, const hal_uart_config_t *cfg) {
  if (cfg == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_uart_ops.init(uart, cfg);
}

hal_status_t hal_uart_enable_interrupt(hal_uart_t uart, uint8_t rx_en,
                                       uint8_t tx_en) {
  return _hal_uart_ops.enable_interrupt(uart, rx_en, tx_en);
}

hal_status_t hal_uart_write(hal_uart_t uart, const uint8_t *data,
                            uint16_t length) {
  if (data == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_uart_ops.write(uart, data, length);
}

hal_status_t hal_uart_write_char(hal_uart_t uart, char c) {
  return _hal_uart_ops.write_char(uart, c);
}

hal_status_t hal_uart_write_int(hal_uart_t uart, int32_t num) {
  return _hal_uart_ops.write_int(uart, num);
}

hal_status_t hal_uart_write_uint(hal_uart_t uart, uint32_t num) {
  return _hal_uart_ops.write_uint(uart, num);
}

hal_status_t hal_uart_write_float(hal_uart_t uart, float num) {
  return _hal_uart_ops.write_float(uart, num);
}

hal_status_t hal_uart_write_string(hal_uart_t uart, const char *s) {
  if (s == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_uart_ops.write_string(uart, s);
}

char hal_uart_read_char(hal_uart_t uart) {
  return _hal_uart_ops.read_char(uart);
}

bool hal_uart_available(hal_uart_t uart) {
  return _hal_uart_ops.available(uart);
}

uint32_t hal_uart_read_until(hal_uart_t uart, char *buffer, uint32_t maxlen,
                             char delimiter) {
  if (buffer == NULL || maxlen == 0)
    return 0;
  return _hal_uart_ops.read_until(uart, buffer, maxlen, delimiter);
}
