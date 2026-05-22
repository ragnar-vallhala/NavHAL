/**
 * @file src/vendor/microchip/uart/uart.c
 * @brief ATmega328P UART HAL driver (USART0, polling mode).
 *
 * @details
 * Implements @c common/hal_uart.h for the ATmega328P's single USART,
 * exposed as ::HAL_UART_0. All transfers are blocking / polling-mode; the
 * frame format is fixed at 8N1.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#include "common/hal_uart.h"
#include "navhal_port_interrupt.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Reject any UART id other than the one USART0. */
static inline bool uart_valid(hal_uart_t uart) { return uart == HAL_UART_0; }

/** @brief Render an unsigned 32-bit value as a decimal string. */
static void u32_to_str(uint32_t v, char *buf) {
  char tmp[10];
  uint8_t n = 0;
  do {
    tmp[n++] = (char)('0' + (v % 10u));
    v /= 10u;
  } while (v != 0u);
  uint8_t j = 0;
  while (n != 0u)
    buf[j++] = tmp[--n];
  buf[j] = '\0';
}

hal_status_t hal_uart_init(hal_uart_t uart, const hal_uart_config_t *cfg) {
  if (!uart_valid(uart) || cfg == NULL || cfg->baudrate == 0u)
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

hal_status_t hal_uart_enable_interrupt(hal_uart_t uart, uint8_t rx_en,
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

hal_status_t hal_uart_write_char(hal_uart_t uart, char c) {
  if (!uart_valid(uart))
    return HAL_ERR_INVALID_ARG;
  while (!(UCSR0A & (1u << UDRE0)))
    ; /* wait for the transmit buffer to drain. */
  UDR0 = (uint8_t)c;
  return HAL_OK;
}

hal_status_t hal_uart_write(hal_uart_t uart, const uint8_t *data,
                            uint16_t length) {
  if (!uart_valid(uart) || data == NULL)
    return HAL_ERR_INVALID_ARG;
  for (uint16_t i = 0; i < length; i++)
    (void)hal_uart_write_char(uart, (char)data[i]);
  return HAL_OK;
}

hal_status_t hal_uart_write_string(hal_uart_t uart, const char *s) {
  if (!uart_valid(uart) || s == NULL)
    return HAL_ERR_INVALID_ARG;
  while (*s != '\0')
    (void)hal_uart_write_char(uart, *s++);
  return HAL_OK;
}

hal_status_t hal_uart_write_uint(hal_uart_t uart, uint32_t num) {
  if (!uart_valid(uart))
    return HAL_ERR_INVALID_ARG;
  char buf[11];
  u32_to_str(num, buf);
  return hal_uart_write_string(uart, buf);
}

hal_status_t hal_uart_write_int(hal_uart_t uart, int32_t num) {
  if (!uart_valid(uart))
    return HAL_ERR_INVALID_ARG;
  uint32_t mag;
  if (num < 0) {
    (void)hal_uart_write_char(uart, '-');
    /* Two's-complement magnitude — correct even for INT32_MIN. */
    mag = (uint32_t)0 - (uint32_t)num;
  } else {
    mag = (uint32_t)num;
  }
  return hal_uart_write_uint(uart, mag);
}

hal_status_t hal_uart_write_float(hal_uart_t uart, float num) {
  if (!uart_valid(uart))
    return HAL_ERR_INVALID_ARG;
  if (num < 0.0f) {
    (void)hal_uart_write_char(uart, '-');
    num = -num;
  }
  uint32_t ip = (uint32_t)num;
  uint32_t fp = (uint32_t)((num - (float)ip) * 1000.0f + 0.5f);
  (void)hal_uart_write_uint(uart, ip);
  (void)hal_uart_write_char(uart, '.');
  if (fp < 100u)
    (void)hal_uart_write_char(uart, '0');
  if (fp < 10u)
    (void)hal_uart_write_char(uart, '0');
  return hal_uart_write_uint(uart, fp);
}

char hal_uart_read_char(hal_uart_t uart) {
  if (!uart_valid(uart))
    return 0;
  while (!(UCSR0A & (1u << RXC0)))
    ; /* wait for a received byte. */
  return (char)UDR0;
}

bool hal_uart_available(hal_uart_t uart) {
  if (!uart_valid(uart))
    return false;
  return (UCSR0A & (1u << RXC0)) != 0u;
}

uint32_t hal_uart_read_until(hal_uart_t uart, char *buffer, uint32_t maxlen,
                             char delimiter) {
  if (!uart_valid(uart) || buffer == NULL || maxlen == 0u)
    return 0;
  uint32_t n = 0;
  while (n < maxlen - 1u) {
    char c = hal_uart_read_char(uart);
    if (c == delimiter)
      break;
    buffer[n++] = c;
  }
  buffer[n] = '\0';
  return n;
}

/* USART0 receive-complete interrupt — routed to the callback registered via
 * hal_interrupt_attach_callback(HAL_IRQ_USART_RX, ...). The callback must
 * consume the byte (hal_uart_read_char) so the interrupt does not re-fire. */
ISR(USART_RX_vect) { hal_interrupt_dispatch(HAL_IRQ_USART_RX); }
