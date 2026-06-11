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
 * @file navtest_target.h
 * @brief Per-arch aliases for the navtest runner so tests/main.c stays
 *        target-agnostic.
 *
 * @details
 * The navtest framework needs ONE concrete UART instance for its output.
 * Cortex-M (Nucleo-F401RE) routes USART2 to the ST-LINK virtual COM; the
 * ATmega328P has a single USART0. Whichever public `hal_uart_t` enum value
 * names the convention's "console UART" on this build's target is what
 * `NAVTEST_UART` evaluates to. Add a branch here when a new arch lands.
 */
#ifndef NAVTEST_TARGET_H
#define NAVTEST_TARGET_H

#include "common/hal_uart.h"

#if defined(__AVR__)
#  define NAVTEST_UART HAL_UART_0   /* USART0 — only USART on ATmega328P */
#elif defined(__arm__) || defined(__thumb__)
#  define NAVTEST_UART HAL_UART_2   /* Nucleo-F401RE → ST-LINK virtual COM */
#else
#  error "navtest_target.h: no NAVTEST_UART mapping for this arch; add a branch."
#endif

#endif /* NAVTEST_TARGET_H */
