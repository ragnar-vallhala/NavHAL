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
 * @file uart_types.h
 * @brief UART peripheral-instance identifier — Cortex-M4 / STM32F4 port.
 *
 * @details
 * The set of valid UART instances is target-defined, so ::hal_uart_t is
 * port-resolved: each port ships its own @c utils/uart_types.h and the
 * `-I include/port/<processor>` path selects it. The portable UART API in
 * @c common/hal_uart.h takes ::hal_uart_t without assuming any particular
 * instance set.
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
