/*
 * Copyright (C) 2025 NAVRobotec Pvt Ltd
 * Author: Ragnar Vallhala
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file uart.c
 * @brief HAL UART driver for the PC 16550 UART (COM1..COM4), polled TX.
 *
 * @details
 * Implements the blocking transmit path of the standardized hal_uart_* API
 * against the standard PC serial ports. Under QEMU, COM1 (0x3F8) is wired to
 * `-serial stdio`, so hal_uart_write* lands on the host terminal. RX / IRQ
 * paths are stubbed until the 8259 PIC + IDT land.
 */

#include "common/hal_uart.h"
#include "internal/hal_uart_ops.h"
#include "vga/vga.h" /* mirror console output to the on-screen terminal */
#if NAVHAL_CONFIG_DRV_INTERRUPT
#include "ps2/keyboard.h" /* screen-terminal input: PC keyboard -> console RX */
#endif

/* Standard PC serial base I/O ports. */
static uint16_t uart_base(hal_uart_t uart) {
  switch (uart) {
  case HAL_UART_1: return 0x3F8; /* COM1 */
  case HAL_UART_2: return 0x2F8; /* COM2 */
  case HAL_UART_3: return 0x3E8; /* COM3 */
  case HAL_UART_4: return 0x2E8; /* COM4 */
  default:         return 0;
  }
}

static inline void outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
  uint8_t r;
  __asm__ volatile("inb %1, %0" : "=a"(r) : "Nd"(port));
  return r;
}

static hal_status_t pc_uart_init(hal_uart_t uart,
                                 const hal_uart_config_t *cfg) {
  if (!cfg) return HAL_ERR_INVALID_ARG;
  uint16_t base = uart_base(uart);
  if (!base) return HAL_ERR_INVALID_ARG;

  /* 16550 divisor from the fixed 115200 max baud clock. */
  uint32_t baud = cfg->baudrate ? cfg->baudrate : 115200u;
  uint16_t div = (uint16_t)(115200u / baud);
  if (div == 0) div = 1;

  outb(base + 1, 0x00);              /* disable interrupts */
  outb(base + 3, 0x80);              /* DLAB: access divisor */
  outb(base + 0, (uint8_t)(div & 0xFF));
  outb(base + 1, (uint8_t)((div >> 8) & 0xFF));
  outb(base + 3, 0x03);              /* 8N1, DLAB off */
  outb(base + 2, 0xC7);              /* FIFO on, cleared, 14-byte threshold */
  outb(base + 4, 0x0B);             /* DTR/RTS on, OUT2 (needed for IRQs later) */
  vga_init();                        /* clear the on-screen terminal */
#if NAVHAL_CONFIG_DRV_INTERRUPT
  kbd_init();                        /* accept keystrokes from the PC keyboard */
#endif
  return HAL_OK;
}

static hal_status_t pc_uart_write_char(hal_uart_t uart, char c) {
  uint16_t base = uart_base(uart);
  if (!base) return HAL_ERR_INVALID_ARG;
  while ((inb(base + 5) & 0x20) == 0) { /* wait: THR empty */ }
  outb(base, (uint8_t)c);
  vga_putc(c); /* mirror to the on-screen terminal */
  return HAL_OK;
}



static void uart_write_dec(hal_uart_t uart, uint32_t v) {
  char buf[10];
  int i = 0;
  if (v == 0) {
    (void)hal_uart_write_char(uart, '0');
    return;
  }
  while (v) {
    buf[i++] = (char)('0' + (v % 10));
    v /= 10;
  }
  while (i--) (void)hal_uart_write_char(uart, buf[i]);
}



/* ===== Receive (polled) ===== */

static bool pc_uart_available(hal_uart_t uart) {
  uint16_t base = uart_base(uart);
  if (!base) return false;
#if NAVHAL_CONFIG_DRV_INTERRUPT
  if (kbd_available()) return true; /* PC keyboard input */
#endif
  return (inb(base + 5) & 0x01) != 0; /* LSR bit0: Data Ready */
}

static char pc_uart_read_char(hal_uart_t uart) {
  uint16_t base = uart_base(uart);
  if (!base) return 0;
  for (;;) {
#if NAVHAL_CONFIG_DRV_INTERRUPT
    int k = kbd_getchar(); /* keyboard (screen terminal) */
    if (k >= 0) return (char)k;
#endif
    if (inb(base + 5) & 0x01) return (char)inb(base); /* serial RBR */
  }
}

/* The 16550 RX interrupt path is not wired up (the port polls), so this
 * reports the capability honestly rather than silently doing nothing. */
static hal_status_t pc_uart_enable_interrupt(hal_uart_t uart, uint8_t rx_en,
                                             uint8_t tx_en) {
  (void)uart;
  (void)rx_en;
  (void)tx_en;
  return HAL_ERR_NOT_SUPPORTED;
}

/** @brief The PC UART primitives; derived writes come from the shared layer. */
const hal_uart_ops_t _hal_uart_ops = {
    .init = pc_uart_init,
    .enable_interrupt = pc_uart_enable_interrupt,
    .write_char = pc_uart_write_char,
    .read_char = pc_uart_read_char,
    .available = pc_uart_available,
};
