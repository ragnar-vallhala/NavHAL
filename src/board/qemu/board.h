/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file board.h
 * @brief Board-layer aliases for the QEMU x86-64 PC.
 *
 * @details
 * A PC has no GPIO/LED pins, so this board defines only the console UART —
 * COM1, which QEMU exposes via `-serial`.
 */

#ifndef NAVHAL_BOARD_QEMU_H
#define NAVHAL_BOARD_QEMU_H

#include "utils/uart_types.h"

/** Console UART: COM1 (0x3F8), wired to QEMU's -serial. */
#define CONSOLE_UART HAL_UART_1

#endif /* NAVHAL_BOARD_QEMU_H */
