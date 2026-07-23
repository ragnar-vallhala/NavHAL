/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file vga.c
 * @brief x86 VGA text-mode console — 80x25 cells at 0xB8000.
 *
 * @details
 * GRUB leaves the display in VGA text mode 3 (multiboot2 with no framebuffer
 * request), so each screen cell is a {character, attribute} 16-bit word at
 * physical 0xB8000 — identity-mapped by the boot page tables. This is the
 * on-screen half of the console; the UART driver mirrors output here.
 */

#include "vga/vga.h"
#include "pc_io.h"
#include <stdint.h>

#define VGA_MEM ((volatile uint16_t *)0xB8000)
#define VGA_COLS 80
#define VGA_ROWS 25
#define VGA_ATTR 0x07 /* light grey on black */
#define VGA_CELL(ch) (((uint16_t)VGA_ATTR << 8) | (uint8_t)(ch))

static int s_row;
static int s_col;
static int s_inited;

/* Point the hardware cursor at (s_row, s_col) via the CRTC registers. */
static void move_cursor(void) {
  uint16_t pos = (uint16_t)(s_row * VGA_COLS + s_col);
  pc_outb(0x3D4, 0x0F);
  pc_outb(0x3D5, (uint8_t)(pos & 0xFF));
  pc_outb(0x3D4, 0x0E);
  pc_outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_init(void) {
  if (s_inited) return;
  s_inited = 1;
  for (int i = 0; i < VGA_COLS * VGA_ROWS; i++)
    VGA_MEM[i] = VGA_CELL(' ');
  s_row = 0;
  s_col = 0;
  move_cursor();
}

static void scroll(void) {
  for (int i = 0; i < (VGA_ROWS - 1) * VGA_COLS; i++)
    VGA_MEM[i] = VGA_MEM[i + VGA_COLS];
  for (int c = 0; c < VGA_COLS; c++)
    VGA_MEM[(VGA_ROWS - 1) * VGA_COLS + c] = VGA_CELL(' ');
  s_row = VGA_ROWS - 1;
}

void vga_putc(char ch) {
  if (!s_inited) vga_init();

  switch (ch) {
  case '\n':
    s_col = 0;
    s_row++;
    break;
  case '\r':
    s_col = 0;
    break;
  case '\t':
    s_col = (s_col + 8) & ~7;
    break;
  default:
    VGA_MEM[s_row * VGA_COLS + s_col] = VGA_CELL(ch);
    s_col++;
    break;
  }

  if (s_col >= VGA_COLS) {
    s_col = 0;
    s_row++;
  }
  if (s_row >= VGA_ROWS) scroll();
  move_cursor();
}

void vga_puts(const char *s) {
  while (*s) vga_putc(*s++);
}
