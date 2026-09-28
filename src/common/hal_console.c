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
 * @file hal_console.c
 * @brief Console route dispatch — see hal_console.h for why this is board-owned.
 */

#include "common/hal_console.h"
#include "board.h"

/* CONSOLE_ROUTE_CDC depends on DRV_USB_CDC in Kconfig, so the invalid pairing is
 * unrepresentable here and needs no guard. */
#if NAVHAL_CONFIG_CONSOLE_ROUTE_CDC
#include "common/hal_usb_cdc.h"
#endif

/* Digits for the longest 32-bit decimal, plus NUL. */
#define CONSOLE_DEC_MAX 11u

/* A board that declares no console UART has nowhere for the UART route to go.
 * Writes are dropped rather than failing the build -- a board can be useful
 * without a console, and the reference and QEMU boards are -- and
 * hal_console_connected() reports false so a caller can tell. */
#if !NAVHAL_CONFIG_CONSOLE_ROUTE_CDC && !defined(BOARD_CONSOLE_UART)
#define CONSOLE_NO_TRANSPORT 1
#else
#define CONSOLE_NO_TRANSPORT 0
#endif

static void console_put(const char *s) {
#if CONSOLE_NO_TRANSPORT
  (void)s;
#elif NAVHAL_CONFIG_CONSOLE_ROUTE_CDC
  uint16_t n = 0u;
  while (s[n] != '\0') {
    n++;
  }
  /* Dropped when no host is attached, which is the honest outcome: there is
   * nowhere for it to go. hal_console_connected() is how a caller finds out
   * before it matters. */
  (void)hal_usb_cdc_write((const uint8_t *)s, n);
#else
  hal_uart_write_string(BOARD_CONSOLE_UART, s);
#endif
}

hal_status_t hal_console_init(uint32_t baudrate) {
#if CONSOLE_NO_TRANSPORT
  (void)baudrate;
  return HAL_ERR_NOT_SUPPORTED;
#elif NAVHAL_CONFIG_CONSOLE_ROUTE_CDC
  (void)baudrate; /* USB negotiates its own rate. */
  return hal_usb_cdc_init();
#else
  hal_uart_config_t cfg = {.baudrate = baudrate};
  return hal_uart_init(BOARD_CONSOLE_UART, &cfg);
#endif
}

bool hal_console_connected(void) {
#if CONSOLE_NO_TRANSPORT
  return false;
#elif NAVHAL_CONFIG_CONSOLE_ROUTE_CDC
  return hal_usb_cdc_connected();
#else
  /* A UART transmits whether or not anything is listening. */
  return true;
#endif
}

hal_console_route_t hal_console_get_route(void) {
#if NAVHAL_CONFIG_CONSOLE_ROUTE_CDC
  return HAL_CONSOLE_ROUTE_CDC;
#else
  return HAL_CONSOLE_ROUTE_UART;
#endif
}

void hal_console_write(const char *s) {
  if (s != NULL) {
    console_put(s);
  }
}

void hal_console_write_uint(uint32_t v) {
  char buf[CONSOLE_DEC_MAX + 1u];
  unsigned i = CONSOLE_DEC_MAX;
  buf[i] = '\0';
  if (v == 0u) {
    buf[--i] = '0';
  }
  while (v != 0u && i > 0u) {
    buf[--i] = (char)('0' + (v % 10u));
    v /= 10u;
  }
  console_put(buf + i);
}

void hal_console_write_hex32(uint32_t v) {
  static const char digits[] = "0123456789ABCDEF";
  char buf[11];
  buf[0] = '0';
  buf[1] = 'x';
  for (unsigned i = 0u; i < 8u; i++) {
    buf[2u + i] = digits[(v >> (28u - 4u * i)) & 0xFu];
  }
  buf[10] = '\0';
  console_put(buf);
}
