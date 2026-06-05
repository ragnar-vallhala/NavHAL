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
 * @file src/vendor/microchip/uart/uart.c
 * @brief ATmega328P UART vendor backend (USART0, polling mode).
 *
 * @details
 * Provides the ATmega328P implementations behind ::hal_uart_ops_t for the
 * single USART, exposed as ::HAL_UART_0. All transfers are blocking /
 * polling-mode; the frame format is fixed at 8N1. Documented NULL-argument
 * validation lives in the shared public layer src/common/hal_uart.c; the table
 * is published as ::_hal_uart_ops.
 */

#include "internal/hal_uart_ops.h"
#include "navhal_port_interrupt.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Reject any UART id other than the one USART0. */
static inline bool uart_valid(hal_uart_t uart) { return uart == HAL_UART_0; }

static hal_status_t avr_uart_init(hal_uart_t uart,
                                  const hal_uart_config_t *cfg) {
  /* cfg non-NULL: validated by the public layer. */
  if (!uart_valid(uart) || cfg->baudrate == 0u)
    return HAL_ERR_INVALID_ARG;

  /* Double-speed mode (U2X0). At 16 MHz the normal-mode divisor for common
   * high baud rates carries a large error — 115200 lands at 125000 (~8.5%),
   * well outside UART tolerance, which garbles the line. Double speed (8x
   * oversampling) plus a rounded divisor keeps the error in range:
   *   UBRR = round(F_CPU / (8 * baud)) - 1
   * e.g. 115200 @ 16 MHz -> UBRR 16 -> 117647 baud (+2.1%, within tolerance).
   * This matches how the Arduino core configures 115200 at 16 MHz. */
  uint16_t ubrr = (uint16_t)(((F_CPU + 4UL * cfg->baudrate) /
                              (8UL * cfg->baudrate)) -
                             1UL);

  UCSR0A = (uint8_t)(1u << U2X0);                      /* double speed. */
  UBRR0H = (uint8_t)(ubrr >> 8);
  UBRR0L = (uint8_t)(ubrr & 0xFFu);
  UCSR0B = (uint8_t)((1u << RXEN0) | (1u << TXEN0));   /* RX + TX enable. */
  UCSR0C = (uint8_t)((1u << UCSZ01) | (1u << UCSZ00)); /* 8-N-1. */
  return HAL_OK;
}

static hal_status_t avr_uart_enable_interrupt(hal_uart_t uart, uint8_t rx_en,
                                              uint8_t tx_en) {
  if (!uart_valid(uart))
    return HAL_ERR_INVALID_ARG;
  if (rx_en)
    UCSR0B |= (uint8_t)(1u << RXCIE0);
  else
    UCSR0B &= (uint8_t)~(1u << RXCIE0);
  if (tx_en)
    UCSR0B |= (uint8_t)(1u << UDRIE0);
  else
    UCSR0B &= (uint8_t)~(1u << UDRIE0);
  return HAL_OK;
}

static hal_status_t avr_uart_write_char(hal_uart_t uart, char c) {
  if (!uart_valid(uart))
    return HAL_ERR_INVALID_ARG;
  while (!(UCSR0A & (1u << UDRE0)))
    ; /* wait for the transmit buffer to drain. */
  UDR0 = (uint8_t)c;
  return HAL_OK;
}

static char avr_uart_read_char(hal_uart_t uart) {
  if (!uart_valid(uart))
    return 0;
  while (!(UCSR0A & (1u << RXC0)))
    ; /* wait for a received byte. */
  return (char)UDR0;
}

static bool avr_uart_available(hal_uart_t uart) {
  if (!uart_valid(uart))
    return false;
  return (UCSR0A & (1u << RXC0)) != 0u;
}

const hal_uart_ops_t _hal_uart_ops = {
    .init = avr_uart_init,
    .enable_interrupt = avr_uart_enable_interrupt,
    .write_char = avr_uart_write_char,
    .read_char = avr_uart_read_char,
    .available = avr_uart_available,
};

/* USART0 receive-complete interrupt — routed to the callback registered via
 * hal_interrupt_attach_callback(HAL_IRQ_USART_RX, ...). The callback must
 * consume the byte (hal_uart_read_char) so the interrupt does not re-fire. */
ISR(USART_RX_vect) { hal_interrupt_dispatch(HAL_IRQ_USART_RX); }
