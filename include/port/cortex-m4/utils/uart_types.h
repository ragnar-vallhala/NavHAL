/**
 * @file uart_types.h
 * @brief UART peripheral-instance identifier — Cortex-M4 / STM32F4 port.
 *
 * @details
 * The set of valid UART instances is target-defined, so ::hal_uart_t is
 * port-resolved: each port ships its own @c utils/uart_types.h and the
 * `-I include/port/<processor>` path selects it. The portable UART API in
 * @c common/hal_uart.h takes ::hal_uart_t without assuming any particular
 * instance set.
 *
 * @copyright © NAVROBOTEC PVT. LTD.
 */

#ifndef UART_TYPES_H
#define UART_TYPES_H

#include "common/navhal_compiler.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief UART peripheral instance identifier (STM32F401RE: USART1/2/6).
 */
typedef enum {
  HAL_UART_1 = 1, /**< USART1 — APB2 peripheral. */
  HAL_UART_2 = 2, /**< USART2 — APB1 peripheral. */
  HAL_UART_6 = 6, /**< USART6 — APB2 peripheral. */
  UART1 NAVHAL_DEPRECATED("use HAL_UART_1") = 1,
  UART2 NAVHAL_DEPRECATED("use HAL_UART_2") = 2,
  UART6 NAVHAL_DEPRECATED("use HAL_UART_6") = 6,
} hal_uart_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* UART_TYPES_H */
