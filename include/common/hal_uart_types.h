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
 * @file hal_uart_types.h
 * @brief UART types, split out so the ops table and the port header can
 *        have them without including the API.
 *
 * common/hal_uart.h ends by including the port header, and everything behind
 * that header -- the family code and the ops table -- needs these types. Taking
 * them from the API header made the include graph circular, which clang-tidy
 * reports at every call site that opens one. Include guards made it compile;
 * they did not make it a graph anyone could follow.
 */
#ifndef HAL_UART_TYPES_H
#define HAL_UART_TYPES_H

#include "common/hal_status.h"
#include "common/navhal_compiler.h"
#include "utils/uart_types.h" /* port-resolved ::hal_uart_t instance enum */
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief UART configuration passed to ::hal_uart_init. */
typedef struct {
  uint32_t baudrate; /**< Baud rate in bits per second. */
} hal_uart_config_t;

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* HAL_UART_TYPES_H */
