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

hal_status_t hal_uart_init(hal_uart_t uart, const hal_uart_config_t *cfg) {
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
  return HAL_OK;
}

hal_status_t hal_uart_write_char(hal_uart_t uart, char c) {
  uint16_t base = uart_base(uart);
  if (!base) return HAL_ERR_INVALID_ARG;
  while ((inb(base + 5) & 0x20) == 0) { /* wait: THR empty */ }
  outb(base, (uint8_t)c);
  return HAL_OK;
}

hal_status_t hal_uart_write(hal_uart_t uart, const uint8_t *data,
                            uint16_t length) {
  if (!data) return HAL_ERR_INVALID_ARG;
  if (!uart_base(uart)) return HAL_ERR_INVALID_ARG;
  for (uint16_t i = 0; i < length; i++)
    (void)hal_uart_write_char(uart, (char)data[i]);
  return HAL_OK;
}

hal_status_t hal_uart_write_string(hal_uart_t uart, const char *s) {
  if (!s) return HAL_ERR_INVALID_ARG;
  if (!uart_base(uart)) return HAL_ERR_INVALID_ARG;
  while (*s) (void)hal_uart_write_char(uart, *s++);
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

hal_status_t hal_uart_write_uint(hal_uart_t uart, uint32_t num) {
  if (!uart_base(uart)) return HAL_ERR_INVALID_ARG;
  uart_write_dec(uart, num);
  return HAL_OK;
}

hal_status_t hal_uart_write_int(hal_uart_t uart, int32_t num) {
  if (!uart_base(uart)) return HAL_ERR_INVALID_ARG;
  if (num < 0) {
    (void)hal_uart_write_char(uart, '-');
    uart_write_dec(uart, (uint32_t)(-(int64_t)num));
  } else {
    uart_write_dec(uart, (uint32_t)num);
  }
  return HAL_OK;
}
