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
 * @brief UART peripheral-instance identifier — AVR / ATmega328P port.
 *
 * @details
 * The ATmega328P has a single USART, exposed as ::HAL_UART_0.
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
