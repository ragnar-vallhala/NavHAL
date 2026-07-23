/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file vga.h
 * @brief x86 VGA text-mode console (0xB8000), 80x25.
 *
 * @details
 * Not part of the portable hal_* API — VGA has no MCU equivalent. It exists so
 * the QEMU/PC *screen* shows console output: the UART driver mirrors each
 * transmitted character here, giving an always-on on-screen terminal alongside
 * the serial line.
 */

#ifndef NAVHAL_PC_VGA_H
#define NAVHAL_PC_VGA_H

/** @brief Clear the screen and home the cursor (idempotent). */
void vga_init(void);

/** @brief Write one character, handling \n, \r, \t, and scrolling. */
void vga_putc(char c);

/** @brief Write a null-terminated string. */
void vga_puts(const char *s);

#endif /* NAVHAL_PC_VGA_H */
