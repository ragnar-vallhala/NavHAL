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
 * @file hal_console.h
 * @brief Where console output goes, chosen by the board rather than the caller.
 *
 * @details
 * A console is a board property, not a driver one. Most boards carry theirs on a
 * UART that a debug probe exposes as a virtual COM port; a flight controller has
 * no room for that header and carries it on its USB device port instead. Callers
 * that print should not have to know which.
 *
 * The transport is a Kconfig choice, @c CONSOLE_ROUTE_UART or
 * @c CONSOLE_ROUTE_CDC, so selecting one is a configuration change rather than a
 * code change: the same board can be built either way. ::hal_console_route_t is
 * the runtime view of that choice.
 *
 * ### The SWD capture depends on the UART route
 *
 * `tools/ntest` can test a board whose console goes nowhere by breaking on
 * @c hal_uart_write_string and reading the argument out of the target
 * (`_swd_capture`). That only sees output that actually passes through the UART
 * driver. A board routed to CDC produces nothing for it to break on, so
 * `CONSOLE=` in `tools/hil/boards/<board>.conf` has to agree with the route the
 * firmware was built with — they are two halves of one decision.
 *
 * @defgroup HAL_CONSOLE Console
 * @ingroup HAL_CORE
 * @{
 */
#ifndef HAL_CONSOLE_H
#define HAL_CONSOLE_H

#include "common/hal_status.h"
#include "common/hal_uart.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Which transport the console is routed to.
 *
 * An enum rather than a pair of macros so a new transport — SWO, semihosting, a
 * radio link — is one enumerator and one dispatch arm, and so callers can log or
 * report the route they got.
 */
typedef enum {
  HAL_CONSOLE_ROUTE_UART = 0, /**< The board's console UART. */
  HAL_CONSOLE_ROUTE_CDC = 1,  /**< USB device port, via @c hal_usb_cdc. */
} hal_console_route_t;

/**
 * @brief The route this firmware was built with.
 *
 * Fixed at compile time by the Kconfig choice; readable at runtime so a banner
 * can say where it is talking, and so a test can assert it rather than assume.
 */
hal_console_route_t hal_console_get_route(void);

/**
 * @brief Bring the console transport up.
 *
 * On the UART route this initialises @c BOARD_CONSOLE_UART at @p baudrate. On
 * the CDC route @p baudrate is ignored — USB negotiates its own rate — and the
 * call returns as soon as the peripheral is running, which is **not** the same
 * as a host being attached: nothing written before enumeration completes is
 * delivered. A caller that needs its first line seen should wait on
 * ::hal_console_connected.
 *
 * @param baudrate UART baud; ignored on the CDC route.
 * @return ::HAL_OK, or the transport's own failure.
 */
hal_status_t hal_console_init(uint32_t baudrate);

/**
 * @brief Whether the console can currently carry output.
 *
 * Always @c true on the UART route — a UART transmits whether or not anything
 * is listening. On the CDC route it reports enumeration, so it is the honest
 * answer to "will this be seen".
 */
bool hal_console_connected(void);

/** @brief Write a NUL-terminated string to the console. */
void hal_console_write(const char *s);

/** @brief Write an unsigned decimal to the console. */
void hal_console_write_uint(uint32_t v);

/** @brief Write @p v as 0x-prefixed, zero-padded, 32-bit hex. */
void hal_console_write_hex32(uint32_t v);

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */ /* end of group HAL_CONSOLE */
#endif /* HAL_CONSOLE_H */
