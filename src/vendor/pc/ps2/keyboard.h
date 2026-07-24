/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file keyboard.h
 * @brief x86 PS/2 keyboard input (IRQ1) — the input half of the screen terminal.
 *
 * @details
 * Not part of the portable hal_* API. It lets the on-screen (VGA) console accept
 * keystrokes: the UART driver drains this buffer alongside COM1, so console
 * input works from either the serial line or the PC keyboard. Needs the
 * interrupt driver.
 */

#ifndef NAVHAL_PC_KEYBOARD_H
#define NAVHAL_PC_KEYBOARD_H

#include <stdbool.h>

/** @brief Install the IRQ1 handler and enable keyboard input (idempotent). */
void kbd_init(void);

/** @brief True if a decoded character is waiting. */
bool kbd_available(void);

/** @brief Pop the next decoded character, or -1 if none. */
int kbd_getchar(void);

#endif /* NAVHAL_PC_KEYBOARD_H */
