/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file keyboard.c
 * @brief x86 PS/2 keyboard driver — scancode set 1, US layout, IRQ1.
 *
 * @details
 * On each IRQ1 the scancode is read from port 0x60, translated to ASCII (with a
 * shift-state for upper case and shifted symbols) and pushed to a ring buffer
 * that the UART driver drains. Release codes (bit 7) only update the shift
 * state. Enter is delivered as '\r' to match the serial terminal.
 */

#include "ps2/keyboard.h"
#include "pc_io.h"
#include "common/hal_interrupt.h"
#include <stdint.h>

#define KBD_DATA 0x60
#define KBD_STATUS 0x64

/* Scancode set 1 -> ASCII (US), unshifted. 0 = ignored. */
static const char map[128] = {
    [0x02] = '1',  [0x03] = '2',  [0x04] = '3',  [0x05] = '4',  [0x06] = '5',
    [0x07] = '6',  [0x08] = '7',  [0x09] = '8',  [0x0A] = '9',  [0x0B] = '0',
    [0x0C] = '-',  [0x0D] = '=',  [0x0E] = '\b', [0x0F] = '\t', [0x10] = 'q',
    [0x11] = 'w',  [0x12] = 'e',  [0x13] = 'r',  [0x14] = 't',  [0x15] = 'y',
    [0x16] = 'u',  [0x17] = 'i',  [0x18] = 'o',  [0x19] = 'p',  [0x1A] = '[',
    [0x1B] = ']',  [0x1C] = '\r', [0x1E] = 'a',  [0x1F] = 's',  [0x20] = 'd',
    [0x21] = 'f',  [0x22] = 'g',  [0x23] = 'h',  [0x24] = 'j',  [0x25] = 'k',
    [0x26] = 'l',  [0x27] = ';',  [0x28] = '\'', [0x29] = '`',  [0x2B] = '\\',
    [0x2C] = 'z',  [0x2D] = 'x',  [0x2E] = 'c',  [0x2F] = 'v',  [0x30] = 'b',
    [0x31] = 'n',  [0x32] = 'm',  [0x33] = ',',  [0x34] = '.',  [0x35] = '/',
    [0x39] = ' ',
};

/* Shifted variants. */
static const char map_shift[128] = {
    [0x02] = '!',  [0x03] = '@',  [0x04] = '#',  [0x05] = '$',  [0x06] = '%',
    [0x07] = '^',  [0x08] = '&',  [0x09] = '*',  [0x0A] = '(',  [0x0B] = ')',
    [0x0C] = '_',  [0x0D] = '+',  [0x0E] = '\b', [0x0F] = '\t', [0x10] = 'Q',
    [0x11] = 'W',  [0x12] = 'E',  [0x13] = 'R',  [0x14] = 'T',  [0x15] = 'Y',
    [0x16] = 'U',  [0x17] = 'I',  [0x18] = 'O',  [0x19] = 'P',  [0x1A] = '{',
    [0x1B] = '}',  [0x1C] = '\r', [0x1E] = 'A',  [0x1F] = 'S',  [0x20] = 'D',
    [0x21] = 'F',  [0x22] = 'G',  [0x23] = 'H',  [0x24] = 'J',  [0x25] = 'K',
    [0x26] = 'L',  [0x27] = ':',  [0x28] = '"',  [0x29] = '~',  [0x2B] = '|',
    [0x2C] = 'Z',  [0x2D] = 'X',  [0x2E] = 'C',  [0x2F] = 'V',  [0x30] = 'B',
    [0x31] = 'N',  [0x32] = 'M',  [0x33] = '<',  [0x34] = '>',  [0x35] = '?',
    [0x39] = ' ',
};

/* Scancodes for the modifier / lock keys we track. */
#define SC_LSHIFT 0x2A
#define SC_RSHIFT 0x36
#define SC_CAPSLOCK 0x3A
#define SC_EXTENDED 0xE0

static volatile unsigned char rb[256];
static volatile uint8_t rb_head, rb_tail;
static bool shift;    /* held */
static bool caps;     /* locked (toggles on each press) */
static bool extended; /* previous byte was the 0xE0 extended prefix */

static void push(char c) {
  uint8_t next = (uint8_t)(rb_head + 1);
  if (next != rb_tail) { /* drop on overflow */
    rb[rb_head] = (unsigned char)c;
    rb_head = next;
  }
}

static void kbd_isr(void) {
  uint8_t sc = pc_inb(KBD_DATA);

  if (sc == SC_EXTENDED) { /* prefix for arrows, right ctrl/alt, keypad, ... */
    extended = true;
    return;
  }
  if (sc & 0x80) { /* key release */
    sc &= 0x7F;
    if (sc == SC_LSHIFT || sc == SC_RSHIFT) shift = false;
    extended = false;
    return;
  }
  if (extended) { /* ignore extended keys (no console meaning here) */
    extended = false;
    return;
  }
  if (sc == SC_LSHIFT || sc == SC_RSHIFT) {
    shift = true;
    return;
  }
  if (sc == SC_CAPSLOCK) { /* toggle on press only; ignore the release */
    caps = !caps;
    return;
  }

  char c = shift ? map_shift[sc] : map[sc];
  /* Caps Lock affects letters only (not shifted digits/symbols). */
  if (caps) {
    if (c >= 'a' && c <= 'z')
      c = (char)(c - 32);
    else if (c >= 'A' && c <= 'Z')
      c = (char)(c + 32);
  }
  if (c) push(c);
}

void kbd_init(void) {
  while (pc_inb(KBD_STATUS) & 0x01) /* drain stale bytes */
    (void)pc_inb(KBD_DATA);
  hal_interrupt_attach_callback(HAL_IRQ_KEYBOARD, kbd_isr);
  hal_interrupt_enable(HAL_IRQ_KEYBOARD);
}

bool kbd_available(void) { return rb_head != rb_tail; }

int kbd_getchar(void) {
  if (rb_head == rb_tail) return -1;
  unsigned char c = rb[rb_tail];
  rb_tail = (uint8_t)(rb_tail + 1);
  return (int)c;
}
