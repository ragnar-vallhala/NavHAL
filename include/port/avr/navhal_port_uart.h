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
 * @file port/avr/navhal_port_uart.h
 * @brief AVR / ATmega328P UART port header.
 *
 * The public UART API lives in @c common/hal_uart.h, which includes this
 * header. The ATmega328P USART0 has no DMA backend, so unlike the Cortex-M4
 * port there are no DMA-backed UART prototypes here.
 */

#ifndef NAVHAL_PORT_UART_H
#define NAVHAL_PORT_UART_H

#include "common/hal_uart.h"

#endif /* NAVHAL_PORT_UART_H */
