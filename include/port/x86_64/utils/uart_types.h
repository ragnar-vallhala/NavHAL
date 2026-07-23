/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file port/x86_64/utils/uart_types.h
 * @brief x86-64 PC UART instance enum (::hal_uart_t) — the standard COM ports.
 */

#ifndef NAVHAL_PORT_X86_64_UART_TYPES_H
#define NAVHAL_PORT_X86_64_UART_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/** @brief UART instances = the four standard PC serial ports (COM1..COM4). */
typedef enum {
  HAL_UART_1 = 1, /**< COM1 — I/O 0x3F8 (QEMU -serial). */
  HAL_UART_2 = 2, /**< COM2 — I/O 0x2F8. */
  HAL_UART_3 = 3, /**< COM3 — I/O 0x3E8. */
  HAL_UART_4 = 4, /**< COM4 — I/O 0x2E8. */
} hal_uart_t;

#ifdef __cplusplus
}
#endif

#endif /* NAVHAL_PORT_X86_64_UART_TYPES_H */
