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
 * @brief Shared public UART layer over the per-vendor hardware primitives.
 *
 * @details
 * The one implementation of the portable @c hal_uart_* API. The vendor backends
 * provide only five hardware primitives (::hal_uart_ops_t — init, interrupt
 * enable, write_char, read_char, available). Everything else lives here, once:
 *
 *   - argument validation (NULL config / buffer / string),
 *   - buffer transmit, decimal integer / unsigned / float / string formatting,
 *   - delimited reads.
 *
 * This is where M9's "~80 % less per-vendor boilerplate" is actually realised —
 * the formatting and framing logic that used to be copy-pasted into each
 * vendor's @c uart.c now has a single home, and a new UART port inherits it by
 * supplying the five primitives.
 *
 * Per-vendor newline policy is preserved: it lives in each backend's
 * @c write_char primitive (the STM32 driver expands @c '\\n' to CR-LF; the AVR
 * driver does not), so the shared formatters compose on top of it unchanged.
 */

#include "common/hal_uart.h"
#include "internal/hal_uart_ops.h"

#include <stddef.h>

/* ----- primitives: thin validate-and-dispatch wrappers ------------------- */

hal_status_t hal_uart_init(hal_uart_t uart, const hal_uart_config_t *cfg) {
  if (cfg == NULL)
    return HAL_ERR_INVALID_ARG;
  return _hal_uart_ops.init(uart, cfg);
}

hal_status_t hal_uart_enable_interrupt(hal_uart_t uart, uint8_t rx_en,
                                       uint8_t tx_en) {
  return _hal_uart_ops.enable_interrupt(uart, rx_en, tx_en);
}

hal_status_t hal_uart_write_char(hal_uart_t uart, char c) {
  return _hal_uart_ops.write_char(uart, c);
}

char hal_uart_read_char(hal_uart_t uart) {
  return _hal_uart_ops.read_char(uart);
}

bool hal_uart_available(hal_uart_t uart) {
  return _hal_uart_ops.available(uart);
}

/* ----- shared logic: built once on top of the primitives ----------------- */

hal_status_t hal_uart_write(hal_uart_t uart, const uint8_t *data,
                            uint16_t length) {
  if (data == NULL)
    return HAL_ERR_INVALID_ARG;
  for (uint16_t i = 0; i < length; i++)
    (void)_hal_uart_ops.write_char(uart, (char)data[i]);
  return HAL_OK;
}

hal_status_t hal_uart_write_string(hal_uart_t uart, const char *s) {
  if (s == NULL)
    return HAL_ERR_INVALID_ARG;
  while (*s != '\0')
    (void)_hal_uart_ops.write_char(uart, *s++);
  return HAL_OK;
}

hal_status_t hal_uart_write_uint(hal_uart_t uart, uint32_t num) {
  char buf[10];
  uint8_t n = 0;
  do {
    buf[n++] = (char)('0' + (num % 10u));
    num /= 10u;
  } while (num != 0u);
  while (n != 0u)
    (void)_hal_uart_ops.write_char(uart, buf[--n]);
  return HAL_OK;
}

hal_status_t hal_uart_write_int(hal_uart_t uart, int32_t num) {
  uint32_t mag;
  if (num < 0) {
    (void)_hal_uart_ops.write_char(uart, '-');
    /* Two's-complement magnitude — correct even for INT32_MIN. */
    mag = (uint32_t)0 - (uint32_t)num;
  } else {
    mag = (uint32_t)num;
  }
  return hal_uart_write_uint(uart, mag);
}

hal_status_t hal_uart_write_float(hal_uart_t uart, float num) {
  /* Canonical format: sign, integer part, '.', 3 zero-padded fractional
   * digits (rounded). Previously the STM32 backend emitted 5 digits and the
   * AVR backend 3 — a silent divergence this single implementation removes. */
  if (num < 0.0f) {
    (void)_hal_uart_ops.write_char(uart, '-');
    num = -num;
  }
  uint32_t ip = (uint32_t)num;
  uint32_t fp = (uint32_t)((num - (float)ip) * 1000.0f + 0.5f);
  if (fp >= 1000u) { /* rounding carried into the integer part. */
    ip += 1u;
    fp -= 1000u;
  }
  (void)hal_uart_write_uint(uart, ip);
  (void)_hal_uart_ops.write_char(uart, '.');
  if (fp < 100u)
    (void)_hal_uart_ops.write_char(uart, '0');
  if (fp < 10u)
    (void)_hal_uart_ops.write_char(uart, '0');
  return hal_uart_write_uint(uart, fp);
}

uint32_t hal_uart_read_until(hal_uart_t uart, char *buffer, uint32_t maxlen,
                             char delimiter) {
  if (buffer == NULL || maxlen == 0)
    return 0;
  uint32_t n = 0;
  while (n < maxlen - 1u) {
    char c = _hal_uart_ops.read_char(uart);
    if (c == delimiter)
      break;
    buffer[n++] = c;
  }
  buffer[n] = '\0';
  return n;
}
