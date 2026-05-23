/**
 * @file uart_types.h
 * @brief UART peripheral-instance identifier — AVR / ATmega328P port.
 *
 * @details
 * The ATmega328P has a single USART, exposed as ::HAL_UART_0.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef UART_TYPES_H
#define UART_TYPES_H


#ifdef __cplusplus
extern "C" {
#endif

/** @brief UART peripheral instance identifier (ATmega328P: USART0). */
typedef enum {
  HAL_UART_0 = 0, /**< USART0 — the only USART on the ATmega328P. */
} hal_uart_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* UART_TYPES_H */
