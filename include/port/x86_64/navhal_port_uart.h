/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file port/x86_64/navhal_port_uart.h
 * @brief x86-64 UART port header.
 *
 * @details
 * The public hal_uart_* prototypes live in @c common/hal_uart.h. The PC 16550
 * driver needs no inline hot-path accessors or register-bit defines here, so
 * this header is intentionally empty (included at the bottom of the common
 * header for API-shape parity with the other ports).
 */

#ifndef NAVHAL_PORT_X86_64_UART_H
#define NAVHAL_PORT_X86_64_UART_H

/* Intentionally empty — no port-specific UART extras on the PC platform. */

#endif /* NAVHAL_PORT_X86_64_UART_H */
